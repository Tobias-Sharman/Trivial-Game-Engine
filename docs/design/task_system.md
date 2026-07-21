# Task System - Design Notes

This documentation is meant to capture the design of the task system and why it has been designed the way it has, along with what the plans for future development are.

## Contents

- [Overview](#overview)
- [TaskGraph](#taskgraph)
- [TaskState packing](#taskstate-packing)
- [TaskPriorityQueue](#taskpriorityqueue)
- [Worker](#worker)
- [TaskSystem](#tasksystem)
- [Waiting](#waiting)
- [Scheduling policy](#scheduling-policy)
- [Shutdown](#shutdown)
- [Known limitations](#known-limitations)
- [Deferred work](#deferred-work)

---

## Overview

The task system is split into a small number of parts, each with a narrow, single responsibility:

- **`TaskGraph`** - pure graph/state management, keeps a record of all tasks and manages dependencies
- **`TaskPriorityQueue`** - a priority-bucketed FIFO queue with per-bucket locking and weighted batch transfer. Used both as the shared injection queue and (currently, as a placeholder) each worker's local queue
- **`Worker`** - plain per-thread state (thread handle, stop source, local queue, park/wake primitives). No behaviour of its own
- **`TaskSystem`** - owns everything above, drives every worker's loop, and is the only layer that understands scheduling policy (budget, stealing, affinity routing, wait/oversubscription)

It is designed this way to avoid any circular dependencies, which is a personal preference as it leaves stuff easier to manage.

---

## TaskGraph

Tasks live in a fixed-size, page-allocated array of slots (sizes and amounts defined in the task graph header), each guarded by its own `TaskSlotMutex`. Tasks are exposed using a `TaskHandle`, which is an index + generation counter, so a stale handle referring to a reused slot is detected rather than silently aliasing. The use of a task handle means that tasks can be safely interacted with whilst leaving the internals purely internal.

### Creation is atomic, including prerequisite attachment and dispatch

Task creation, prerequisite attachment, and dispatch (deciding `Ready` vs. `Waiting`) all happen inside one method, `createDispatched`, under one continuously-held slot lock. The decision to complete creation as one continuously-held slot lock is to avoid a prior subtle bug from a race where task completion of one prerequisite before another is attached results in an incorrect dispatch attempt of a not yet waiting task. The result of this is no window for concurrent completion to observer half finished tasks.

For adding prerequisites it is now done through `addPrerequisiteLocked`, before a non locked version `addPrerequisite` was used but has been removed since no sensible reason for allowing prerequisite addition post task completion was identified and is ultimately an unsafe usage path that can easily be missed.

### Completion is also one atomic step

Completion, likewise, also used to be in two parts but this resulted in a similar bug so was migrated to a single `completeAndCollectDependants` function. This style of collecting dependants keeps to avoiding locking a large series of tasks and allows for simply collecting a record of tasks that can the be individually accessed and locked safely to update their prerequisite lists.

### Debug cycle detection must reuse the caller's lock, not re-take it

For debugging a check for creating cycles in the graph is provided. It traverses down the graph and looks to see if it could be found in its prerequisites or iteratively their prerequisites too. This check is simply for if the task attempting to be added is already present and would not detect existing cycles, it only would find if addition of a task would create a cycle about itself. Here a very subtle bug - found not by reasoning but by a stack trace - was found wherein the prior implementation did not handle re-locking the mutex on the task called on.

---

## TaskState packing

To reduce the size of the task state, and improve cache density, `status`, `priority`, `affinity`, and `lifetime` are packed into a single `std::uint16_t` bitfield (3 + 3 + 3 + 1 = 10 bits used, 6 spare for future changes), accessed through named getter/setter methods rather than direct field access. Every internal consumer of these fields (`TaskGraph`, `TaskSystem`) goes through the accessors; nothing reaches into the packed bits directly for safety, the additional operations in shifting and applying masks are ultimately negligible making the reduced size more desirable.

Initially the `priority` was split into a base and effective priority but it was decided that there should be no need for reduction in priority with respect to its original one so they were merged into one.

Even though `affinity` is currently only 2 types there was enough space to spare so 3 bits were set aside for easier reworking later.

`TaskState::waitGroup` is a plain `TaskWaitGroup*` (not owned, not pooled) - see [Waiting](#waiting).

---

## TaskPriorityQueue

Each priority tier (`Background`/`Normal`/`High`/`Critical`) is a `PriorityBucket` - its own `TaskGraphMutex` paired with its own `std::deque<TaskHandle>` - stored as `std::array<PriorityBucket, Count>`. This is deliberately array-of-structs, not struct-of-arrays: every operation touches one bucket's mutex and its dequeue together, so keeping them adjacent in memory is a better fit for the access pattern, and it also means two threads touching *different* priorities never contend with each other at all (the original design had one mutex covering all four tiers, which serialized unrelated-priority operations against each other for no reason).

### Two ways to consume it

- **`tryPop(TaskHandle& outHandle)`** - single-item, scans highest-priority-first. Used anywhere only one item at a time is needed since there is not multiple consumers
- **`tryPopWeightedBatchInto(TaskPriorityQueue& destination)`** - moves a weighted share of each tier directly into another `TaskPriorityQueue` (a worker's local queue), one lock pair per tier, no intermediate storage. Exists purely to amortize lock-acquisition cost: a worker that would otherwise take the shared queue's lock once per task instead takes it once per *batch*, then drains its own (effectively uncontended) local queue for however many tasks the batch contained

Weights (`TaskPriorityWeights`) and `batchSize` are fixed at construction and pre-computed into per-tier share counts (`m_shares`) - never recomputed per call, since the inputs never change after construction.

### Background priority starves under load - on purpose

A few different designs for background tasks were toyed with, for instance the initial intended implementation involved an atomic counter or a semaphore gating the running of a background task coupled with a maximum amount of threads allowed to work on background tasks. The now intended design to sidestep weird park and wake cases along with the minor overhead of tracking and the background tasks being ran is to dedicate some threads to this directly, whilst allowing them to nonetheless freely pick from the `AnyWorker` global injection queues as needed - see [Deferred work](#deferred-work). The current placeholder implementation just relies on the basic scheduling to dispatch adequate amounts of background tasks so as to not starve them.

---

## Worker

Deliberately a **plain struct, no behaviour of its own** - thread handle, stop source, local queue, park/wake state (mutex + condition_variable_any + `WorkerState`). `TaskSystem` owns a `std::deque<Worker>` (not `vector` - `Worker` holds a mutex and condition variable, so it can't be move-constructed, and `deque` is the standard container that never needs to relocate existing elements when it grows) and drives every worker's loop, park, and wake decision directly. Designed this way to avoid circular dependency of calling into the task system in the worker loop and allow for no inheritance for different thread types.

### WorkerState

Three states: `Active` (competing for and running tasks), `Parked` (asleep, eligible to be woken with arbitrary next work), `Waiting` (blocked in `wait()` on a specific `TaskWaitGroup` - **not** eligible for ordinary park/wake bookkeeping; nothing wakes a `Waiting` worker except its own `TaskWaitGroup` completing).

---

## TaskSystem

### Active worker budget: a semaphore, not a counter

The number of workers simultaneously competing for `AnyWorker` work is capped at `m_targetActiveWorkerCount` via a `std::counting_semaphore<>` (`m_activeSlots`), not a mutex-guarded `size_t` counter.

This went through several iterations before landing here. A plain counter (seeded at various values, checked via `count > target`) kept reintroducing a startup race: multiple worker threads reading the same counter before any of them had "settled" into active-or-parked could all observe a stale value simultaneously. A semaphore's `try_acquire()` is an atomic claim, not a read-then-branch - it can never hand out more than `target` permits regardless of how many threads race to acquire at the same instant, which is what a passively-read counter fundamentally can't guarantee no matter how it's seeded.

- A worker holds a permit for its entire active stretch (not re-checked every loop iteration) - released only when it actually parks
- `wakeOneIfUnderTarget` claims a permit *before* waking anyone, handing it to whichever parked worker it wakes; if it can't find a parked worker to give the permit to, it releases it back immediately
- `wait()`'s oversubscription shed/restore is `release()`/`acquire()` on the same semaphore

### `m_parkedWorkerIndices` - still a plain mutex-guarded stack

The semaphore replaces the counting half of the old mechanism only. Something still needs to track which specific worker indices are parked, for `wakeOneIfUnderTarget` to pick from (LIFO - most recently parked is likely still cache-warm). That's `m_parkedIndicesMutex` + `m_parkedWorkerIndices`, unchanged in shape from the original design.

### Worker loop order

Per iteration: own local queue -> (if holding an active slot) shared `AnyWorker` queue via weighted batch-fill, then local queue again -> steal from a sibling's local queue -> give up the slot and park.

`overBudget` (not holding a slot) does **not** skip checking the shared queue and stealing outright - early designs gated those behind the budget check, which meant a worker could go to sleep without ever having looked at the primary supply queue during a startup window where the budget accounting was still settling. The current design: a worker only attempts `AnyWorker`/steal *if it's holding a slot*; if it isn't, it goes straight to parking without touching new intake - draining its own local queue first either way, never leaving already-committed work stranded.

### tryStealTask

Round-robin from `workerIndex + 1` through every sibling, wrapping. Marked as a placeholder since the actual implementation of load balancing is not settled, standard load balancing with some stuff not free and unknown how much traffic each place has would dictate some having but this may be needless overhead (would not be too bad for a custom hash but could still become noticeable under profiling) so needs testing on a proper load - see [Deferred work](#deferred-work).

---

## Waiting

`wait(TaskHandle)` / `wait(std::span<const TaskHandle>)` do, in order:

1. **Fast path**: `isComplete` check
2. **`tryHelpComplete`**: a bounded, iterative (explicit stack, not recursion to avoid stack-overflow risk) depth-first walk of the target's *own* prerequisite chain. Claims and runs anything found `Ready` with matching affinity. Bounded by `waitHelpMaxDepth` (config) purely as a *cost* cap, not a safety one - an explicit stack means no real recursion-depth/stack-overflow risk regardless.
3. **Attach a `TaskWaitGroup`** via `TaskGraph::tryAttachWaiter`, and block on it - with oversubscription (shed the active slot, wake a standby, block, restore the slot on wake) if the calling thread is a pool worker. Unconditional block if external.

The decision to try to help complete tasks even if it results in extra cost for traversing and locking graph nodes is since usage of wait() is to be avoided and thus if it is used it must be some important bounding. That usage being on some important completion, i.e. important enough not to be blocked by just a dependency chain, is taken as wanting to get that task completed, e.g. the main thread saying wait on everything for frame completion should be prioritised over tasks that can be ran at any time and are low priority.

### TaskWaitGroup

A small fan-in primitive: constructed with a count, decremented once per member completion via the static `onMemberComplete`, triggers its internal wait/notify exactly once when the count reaches zero. Replaces what was originally two separate things - a general-purpose `TaskEvent` (wait/trigger, no count) plus a separate countdown mechanism for the span-wait case. Once every real call site converged on needing a countdown (a single-handle wait is just `count == 1`), keeping a separate zero-count `TaskEvent` type around had no remaining justification, so it was folded in to make graph construction consistent and avoid a virtual dispatch.

Deliberately **not** built on `std::counting_semaphore` - a semaphore has no stop-token-aware `acquire()` (needed so a worker's shutdown can interrupt a blocked wait) and would need an N-times-acquire loop to implement "wake once when all N are done" rather than doing it natively.

`TaskState::waitGroup` is a bare pointer to a **stack-local** object (the wait group lives on the waiting call's own stack frame) - no pool, no heap allocation. This means a wait that's abandoned early (interrupted by shutdown) must explicitly detach itself (`TaskGraph::detachWaiterIfUnclaimed`) before its stack frame is destroyed, or `TaskState::waitGroup` would dangle.

### Span-wait's phantom slot

`wait(std::span)` sizes its `TaskWaitGroup` at `tasks.size() + 1`, not `tasks.size()` - the extra slot is held by the waiting thread itself for the duration of the attach loop, released only after every handle has been attached (or compensated for via `AlreadyComplete`). Without it, a task completing *during* the attach loop (before the loop has reached every handle) could bring the count to zero and fire the trigger before every handle was ever attached - the phantom slot guarantees the count can't reach zero mid-loop regardless of how fast other threads complete tasks concurrently.

### Single-waiter-per-handle is enforced, not designed around

`tryAttachWaiter` asserts (debug-only) that a handle doesn't already have a `waitGroup` attached, rather than supporting multiple concurrent
waiters via a linked structure. This was a deliberate simplification - every real use case collapses to "a caller iterating a known set of handles, attaching to each exactly once" (fan-out/fan-in patterns already express "wait for N things" via one `TaskWaitGroup` over N handles, not via N separate waiters on one handle). If two independent external threads ever need to wait on the *same* handle concurrently, that's not supported today and would need addressing at that point.

---

## Scheduling policy

Priority is a strict scan, not a fairness scheme - `TaskPriorityQueue::tryPop` always checks `Critical` before `High` before `Normal` before `Background`.
Combined with weighted batching for the pool's intake (see [TaskPriorityQueue](#taskpriorityqueue)), this means: proportional representation when filling a worker's local queue, strict priority-order when choosing what to run from what's already loaded.

`TaskPriority`'s declared order matters structurally, not just cosmetically - increasing enum value must mean increasing urgency (`Background` lowest, `Critical` highest), because `TaskPriorityQueue`'s descending scan and `TaskPriorityWeights`' field-to-index mapping both depend on it.

`tryHelpComplete`'s traversal order is depth-first (a stack), not breadth-first - chosen because a shallow `Waiting` node's own prerequisites are, by definition, still unresolved (nothing a scheduler could have picked up yet), so a shallow scan finds mostly non-executable nodes; diving straight down one chain reaches an actually-claimable `Ready` leaf with the least wasted traversal.

---

## Shutdown

`~TaskSystem` drains to genuine quiescence **before** requesting any worker to stop - not the other way around. Concretely, in a loop: pump `MainThread`'s ready queue, drain `AnyWorker`'s ready queue directly on the destructing thread, check every worker's `state == Parked`, check both shared queues are empty; repeat until all of that holds simultaneously. Only then: `request_stop()` on every worker, then join every worker.

This ordering (drain-first) was a deliberate choice over the alternative (request-stop-first, let workers finish naturally) specifically because this engine doesn't support any notion of "reject new task creation" - a running task's own `launch()` calls must always be allowed to succeed (a task legitimately spawning something it depends on), so there's no way to make "outstanding task count" a monotonically-shrinking value without first guaranteeing nothing new is arriving from *outside* the system. Drain-first sidesteps this: the only thread that can inject genuinely new (non-child) work is the one currently blocked inside the destructor, so once it's in the drain loop, the total amount of real work in the system can only shrink. This means that any threads not managed by the task system should be dealt with before looking to destroy the task system, on the very likely and intended main thread.

**A task that never terminates will hang the destructor forever.** This is treated as a caller error, not something the engine guards against - the same way a `std::jthread` whose function ignores its `stop_token` forever will hang on join. `wait()`'s stop-token-interrupted path exists for correctness (a worker's *own* thread being torn down while blocked in `wait()`), but is not reachable via a normal drain-first shutdown, since `request_stop()` is never called until quiescence is already reached - by the time it's called, any real `wait()` would already have unblocked via its `TaskWaitGroup` triggering.

The local-queue emptiness check at the end of the drain loop is a `TRIVIAL_ASSERT` meant to check in debugging, not a loop condition - nothing currently routes work onto a worker's own local queue (it's a placeholder `TaskPriorityQueue` fed only by batch-fill from the shared queue), so finding one non-empty at shutdown would mean a real invariant broke, not a normal case to patiently wait out.

---

## Known limitations

- **No cancellation.** `TaskStatus::Cancelled` and the paths that check or it exist throughout `TaskGraph`, but nothing anywhere ever sets it. Scaffolded, not implemented
- **No test for genuine capacity exhaustion at scale**, beyond a dedicated (slow) test that launches `kMaxTaskCount + 1` tasks directly
- **Different thread affinities beyond main and any** - nothing spawns a thread to drain it
- **`ThreadConfig`'s `stackSize`/`priority`/(future) CPU affinity fields are unused.** `std::thread` offers no portable way to set stack size, and OS-level thread priority before the first instruction runs both need a custom thread type to do properly (see [Deferred work](#deferred-work)).

## Deferred work

Once a custom allocator is in place the rest of the task system can be implemented, most of it is having a rewrite of the standard library so a base of a custom allocator, whilst to requisite for everything underpins the full completion of most aspects.

Roughly in the order they'd likely be tackled, though none are blocking anything above:

1. **Custom thread type.** Real stack size, OS-level thread priority, and CPU core affinity, all set at native thread-creation time rather than post-hoc via `native_handle()` fiddling (which can't set stack size or guarantee priority before the first instruction runs at all). Needed before dedicated affinity threads or a background-worker pool can be done properly
2. **Dedicated `MainThread`/`RenderThread` threads**, and a **dedicated, isolated pool for `Background` work** Structurally the same shape of change for all three: a named thread (or small pool), not participating in the general pool's steal/park/active-slot bookkeeping, draining one specific queue. Different thread types will have worker loops
3. **A real work-stealing local queue** for `Worker::localQueue` (currently the same `TaskPriorityQueue` type used for the shared queue, as a placeholder) - a Chase-Lev-style dequeue, cheap owner push/pop from one end, thief steal from the other. This is also the prerequisite for a real cache-locality optimization: routing a newly-unblocked dependant onto the *completing* worker's own local queue rather than always into the shared queue
4. **Better steal-target selection** than the current round-robin
6. **A smaller mutex type** (`task_mutex.h`'s `TaskSlotMutex`/`TaskGraphMutex` are both plain `std::mutex` today, noted as a possible future reduction in per-slot page size), in theory this should be possible from 40 bits to 32 and maybe the actual implementation can be sped up
7. **Continuation-stealing / stack-switching execution model** - a much larger, separate effort (coroutine- or fiber-based task bodies letting a spawn run inline and only pay suspend/resume cost if actually stolen). Recognized as the "other" textbook answer to the child-stealing model this system already uses, not something to build until child-stealing's overhead is an actual, measured problem.

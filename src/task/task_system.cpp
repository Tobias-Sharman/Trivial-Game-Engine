#include <trivial/task/task_system.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <trivial/core/assert.h>
#include <trivial/core/config.h>
#include <trivial/core/log.h>
#include <trivial/core/platform.h>
#include <trivial/core/profile.h>
#include <trivial/core/sync/latch.h>
#include <trivial/core/sync/lock_guard.h>
#include <trivial/core/sync/mutex.h>
#include <trivial/core/thread/thread.h>
#include <trivial/task/task_graph.h>
#include <trivial/task/task_handle.h>
#include <trivial/task/task_launch_options.h>
#include <trivial/task/task_payload.h>
#include <trivial/task/task_status.h>
#include <trivial/task/worker.h>

#include "core/sync/parking_lot.h"

#define TRIVIAL_TASK_SYSTEM_INVALID_WORKER_INDEX (std::numeric_limits<std::size_t>::max())

namespace {

// Could do some arithmetic operation on current thread - offset, where offset
// is the number of designated threads, but in order to protect against thread
// creation not on the main thread in the future (same vein as the given thread
// index being atomic) the cost of the memory for a handful of pointers is
// insignificant. Has the same indirection as going through thread index too so
// gain would only be memory related
//
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
thread_local trivial::task::Worker* g_currentWorker = nullptr;

#if TRIVIAL_ENABLE_ASSERTS
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
thread_local trivial::task::TaskSystem* g_currentWorkerSystem = nullptr;
#endif // TRIVIAL_ENABLE_ASSERTS

} // namespace

namespace trivial::task {

TaskSystem::TaskSystem(const TaskSystemConfig& config)
    : m_targetActiveWorkerCount(static_cast<std::size_t>(config.workers.count))
    , m_activeSlots(m_targetActiveWorkerCount)
    , m_waitHelpMaxDepth(config.waitHelpMaxDepth) {
	TRIVIAL_ASSERT(config.workers.count > 0);

	const std::size_t kWorkerCount
	    = m_targetActiveWorkerCount + static_cast<std::size_t>(config.workers.maxStandbyWorkers);

	for (std::size_t i = 0; i < kWorkerCount; ++i) {
		m_workers.emplace_back();
	}

	const thread::ThreadConfig kThreadConfig{
	    .name = "Worker",
	    .type = thread::ThreadType::Worker,
#if TRIVIAL_PLATFORM_POSIX
	    .stackAllocator = &m_workerStackAllocator,
#endif // TRIVIAL_PLATFORM_POSIX
	};

	for (Worker& worker : m_workers) {
		const thread::ThreadCreateResult kResult
		    = worker.thread.create(kThreadConfig, &TaskSystem::workerThreadEntry, static_cast<void*>(this));

		if (kResult.error != thread::ThreadCreateError::None) [[unlikely]] {
			TRIVIAL_LOG_FATAL_PREFIX("TaskSystem", "Failed to create worker thread");
			std::abort();
		}
	}
}

TaskSystem::~TaskSystem() noexcept {
	TRIVIAL_PROFILE_SCOPE("TaskSystem shutdown");

	{
		TRIVIAL_PROFILE_SCOPE("TaskSystem shutdown: main thread drain");

		runMainThreadReadyTasks();
	}

	{
		TRIVIAL_PROFILE_SCOPE("TaskSystem shutdown: general drain to joining workers");

		for (Worker& worker : m_workers) {
			worker.stopping.store(true, std::memory_order_release);
			(void)sync::activeParkingLot().unparkOne(sync::parkingKey(worker));
		}

		for (;;) {
			runMainThreadReadyTasks();

			if (!tryPopAndRunOneAnyWorkerTask()) {
				break;
			}
		}

		for (Worker& worker : m_workers) {
			if (worker.thread.joinable()) {
				worker.thread.join();
			}
		}
	}

	{
		// If any tasks were spawned post the above general drain
		// NOTE: If for some reason this becomes noticeably long then need an
		// flag
		TRIVIAL_PROFILE_SCOPE("TaskSystem shutdown: single-threaded tail");

		for (;;) {
			runMainThreadReadyTasks();

			if (!tryPopAndRunOneAnyWorkerTask()) {
				break;
			}
		}
	}

#if TRIVIAL_ENABLE_ASSERTS
	for (const Worker& worker : m_workers) {
		TRIVIAL_ASSERT(worker.localQueue.empty());
	}

	TRIVIAL_ASSERT(m_affinityQueues[static_cast<std::size_t>(TaskAffinity::AnyWorker)].empty());
	TRIVIAL_ASSERT(m_affinityQueues[static_cast<std::size_t>(TaskAffinity::MainThread)].empty());

	TRIVIAL_ASSERT(m_activeSlots.debugCount() == m_targetActiveWorkerCount);
	TRIVIAL_ASSERT(m_parkedWorkerIndices.empty());
#endif // TRIVIAL_ENABLE_ASSERTS
}

TaskHandle TaskSystem::launch(TaskPayload payload, const TaskLaunchOptions& options) noexcept {
	const TaskCreateDispatchOutcome kOutcome = m_graph.createDispatched(std::move(payload), {}, options);

	if (kOutcome.createResult != TaskCreateResult::Success) {
		return {};
	}

	if (kOutcome.readiness == TaskReadiness::Ready) {
		enqueueReadyTask(kOutcome.handle, options.affinity, kOutcome.priority);
	}

	return kOutcome.handle;
}

TaskHandle TaskSystem::launch(TaskPayload payload, TaskHandle prerequisite, const TaskLaunchOptions& options) noexcept {
	const std::array<TaskHandle, 1> kPrerequisites{prerequisite};

	const TaskCreateDispatchOutcome kOutcome
	    = m_graph.createDispatched(std::move(payload), std::span<const TaskHandle>{kPrerequisites}, options);

	if (kOutcome.createResult != TaskCreateResult::Success) {
		return {};
	}

	if (kOutcome.readiness == TaskReadiness::Ready) {
		enqueueReadyTask(kOutcome.handle, options.affinity, kOutcome.priority);
	}

	return kOutcome.handle;
}

TaskHandle TaskSystem::launch(TaskPayload payload,
                              std::span<const TaskHandle> prerequisites,
                              const TaskLaunchOptions& options) noexcept {
	const TaskCreateDispatchOutcome kOutcome = m_graph.createDispatched(std::move(payload), prerequisites, options);

	if (kOutcome.createResult != TaskCreateResult::Success) {
		return {};
	}

	if (kOutcome.readiness == TaskReadiness::Ready) {
		enqueueReadyTask(kOutcome.handle, options.affinity, kOutcome.priority);
	}

	return kOutcome.handle;
}

void TaskSystem::wait(TaskHandle task) noexcept {
	TRIVIAL_ASSERT(task.isValid());

	if (isComplete(task)) {
		return;
	}

	if (tryHelpComplete(task, TaskAffinity::AnyWorker, m_waitHelpMaxDepth)) {
		return;
	}

	sync::Latch latch{1};

	const TaskAttachWaiterResult kAttachResult = m_graph.tryAttachWaiter(task, latch);

	if (kAttachResult == TaskAttachWaiterResult::AlreadyComplete) {
		return;
	}

	TRIVIAL_ASSERT(kAttachResult == TaskAttachWaiterResult::Attached);

	const std::size_t kWorkerIndex = tryGetCurrentWorkerIndex();

	if (kWorkerIndex == TRIVIAL_TASK_SYSTEM_INVALID_WORKER_INDEX) {
		latch.wait();
		return;
	}

	Worker& worker = m_workers[kWorkerIndex];

	worker.state.store(WorkerState::Waiting, std::memory_order_relaxed);

	m_activeSlots.release();

	wakeOneIfUnderTarget();

	latch.wait();

	m_activeSlots.acquire();

	worker.state.store(WorkerState::Active, std::memory_order_relaxed);
}

void TaskSystem::wait(std::span<const TaskHandle> tasks) noexcept {
	bool allComplete = true;

	for (const TaskHandle kTask : tasks) {
		TRIVIAL_ASSERT(kTask.isValid());

		if (isComplete(kTask)) {
			continue;
		}

		if (tryHelpComplete(kTask, TaskAffinity::AnyWorker, m_waitHelpMaxDepth)) {
			continue;
		}

		allComplete = false;
	}

	if (allComplete) {
		return;
	}

	// Extra slot to account for race of tasks finshing before all waiters area attached
	sync::Latch latch{tasks.size() + 1};

	for (const TaskHandle kTask : tasks) {
		if (m_graph.tryAttachWaiter(kTask, latch) == TaskAttachWaiterResult::AlreadyComplete) {
			latch.countDown();
		}
	}

	latch.countDown(); // release the phantom slot

	const std::size_t kWorkerIndex = tryGetCurrentWorkerIndex();

	if (kWorkerIndex == TRIVIAL_TASK_SYSTEM_INVALID_WORKER_INDEX) {
		latch.wait();
		return;
	}

	Worker& worker = m_workers[kWorkerIndex];

	worker.state.store(WorkerState::Waiting, std::memory_order_relaxed);

	m_activeSlots.release();

	wakeOneIfUnderTarget();

	latch.wait();

	m_activeSlots.acquire();

	worker.state.store(WorkerState::Active, std::memory_order_relaxed);
}

void* TaskSystem::getResultPointer(TaskHandle handle) noexcept {
	wait(handle);

	return m_graph.getResultPointer(handle);
}

bool TaskSystem::isComplete(TaskHandle task) const noexcept {
	TaskStatus status = TaskStatus::Created;

	if (!m_graph.tryGetStatus(task, status)) {
		return false;
	}

	return status == TaskStatus::Completed || status == TaskStatus::Cancelled;
}

TaskReleaseResult TaskSystem::release(TaskHandle task) noexcept {
	return m_graph.release(task);
}

void TaskSystem::runMainThreadReadyTasks() noexcept {
	constexpr std::size_t kMainThreadIndex = static_cast<std::size_t>(TaskAffinity::MainThread);

	TaskHandle handle{};

	while (m_affinityQueues[kMainThreadIndex].tryPop(handle)) {
		runAndCompleteClaimedTask(handle);
	}
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
std::size_t TaskSystem::tryGetCurrentWorkerIndex() const noexcept {
	if (g_currentWorker == nullptr) {
		return TRIVIAL_TASK_SYSTEM_INVALID_WORKER_INDEX;
	}

	TRIVIAL_ASSERT(g_currentWorkerSystem == this);

	return g_currentWorker->index;
}

void TaskSystem::workerThreadEntry(void* arg) noexcept {
	TaskSystem* system = static_cast<TaskSystem*>(arg);
	const std::size_t kIndex = system->m_nextWorkerStartIndex.fetch_add(1, std::memory_order_relaxed);

	Worker& worker = system->m_workers[kIndex];
	worker.index = kIndex;

	const std::string kThreadName = "Worker " + std::to_string(kIndex);
	thread::Thread::current()->rename(kThreadName.c_str());

	g_currentWorker = &worker;
#if TRIVIAL_ENABLE_ASSERTS
	g_currentWorkerSystem = system;
#endif // TRIVIAL_ENABLE_ASSERTS

	system->runWorkerLoop(kIndex);
}

void TaskSystem::runWorkerLoop(std::size_t workerIndex) {
	Worker& worker = m_workers[workerIndex];

#if TRIVIAL_ENABLE_TRACY
	TRIVIAL_PROFILE_THREAD(thread::Thread::current()->name());
#endif // TRIVIAL_ENABLE_TRACY

	bool holdingSlot = m_activeSlots.tryAcquire();

	for (;;) {
		TaskHandle handle{};

		if (worker.localQueue.tryPop(handle)) {
			runAndCompleteClaimedTask(handle);
			continue;
		}

		if (holdingSlot && tryClaimSharedTask(workerIndex, handle)) {
			runAndCompleteClaimedTask(handle);
			continue;
		}

		if (worker.stopping.load(std::memory_order_acquire)) {
			break;
		}

		if (holdingSlot) {
			m_activeSlots.release();
		}

		holdingSlot = parkWorker(workerIndex);
	}

	if (holdingSlot) {
		m_activeSlots.release();
	}
}

bool TaskSystem::parkWorker(std::size_t workerIndex) noexcept {
	Worker& worker = m_workers[workerIndex];

	worker.state.store(WorkerState::Parked, std::memory_order_relaxed);

	{
		const sync::LockGuard<sync::Mutex> kParkedLock(m_parkedIndicesMutex);
		m_parkedWorkerIndices.push_back(workerIndex);
	}

	constexpr std::size_t kAnyWorkerIndex = static_cast<std::size_t>(TaskAffinity::AnyWorker);

	if (!m_affinityQueues[kAnyWorkerIndex].empty() && m_activeSlots.tryAcquire()) {
		if (!tryRemoveParkedIndex(workerIndex)) {
			m_activeSlots.release();
		}

		worker.state.store(WorkerState::Active, std::memory_order_relaxed);
		return true;
	}

	(void)sync::activeParkingLot().park(sync::parkingKey(worker), [&worker] {
		return worker.state.load(std::memory_order_acquire) != WorkerState::Active
		       && !worker.stopping.load(std::memory_order_relaxed);
	});

	if (worker.state.load(std::memory_order_acquire) == WorkerState::Active) {
		return true;
	}

	return !tryRemoveParkedIndex(workerIndex);
}

void TaskSystem::wakeOneIfUnderTarget() noexcept {
	if (!m_activeSlots.tryAcquire()) {
		return;
	}

	Worker* workerToWake = nullptr;

	{
		const sync::LockGuard<sync::Mutex> kParkedLock(m_parkedIndicesMutex);

		if (!m_parkedWorkerIndices.empty()) {
			workerToWake = &m_workers[m_parkedWorkerIndices.back()];
			m_parkedWorkerIndices.pop_back();
			workerToWake->state.store(WorkerState::Active, std::memory_order_release);
		}
	}

	if (workerToWake == nullptr) {
		m_activeSlots.release();
		return;
	}

	(void)sync::activeParkingLot().unparkOne(sync::parkingKey(*workerToWake));
}

bool TaskSystem::tryRemoveParkedIndex(std::size_t workerIndex) noexcept {
	const sync::LockGuard<sync::Mutex> kParkedLock(m_parkedIndicesMutex);

	for (std::size_t i = 0; i < m_parkedWorkerIndices.size(); ++i) {
		if (m_parkedWorkerIndices[i] == workerIndex) {
			m_parkedWorkerIndices[i] = m_parkedWorkerIndices.back();
			m_parkedWorkerIndices.pop_back();
			return true;
		}
	}

	return false;
}

bool TaskSystem::tryClaimSharedTask(std::size_t workerIndex, TaskHandle& handle) noexcept {
	Worker& worker = m_workers[workerIndex];

	constexpr std::size_t kAnyWorkerIndex = static_cast<std::size_t>(TaskAffinity::AnyWorker);

	const std::size_t kGranted = m_affinityQueues[kAnyWorkerIndex].tryPopWeightedBatchInto(worker.localQueue);
	if (kGranted > 0 && worker.localQueue.tryPop(handle)) {
		return true;
	}

	return tryStealTask(workerIndex, handle);
}

bool TaskSystem::tryStealTask(std::size_t workerIndex, TaskHandle& handle) noexcept {
	const std::size_t kWorkerCount = m_workers.size();

	if (kWorkerCount <= 1) {
		return false;
	}

	// TODO: More even load balancing whilst staying light weight - custom hash?
	for (std::size_t offset = 1; offset < kWorkerCount; ++offset) {
		const std::size_t kCandidateIndex = (workerIndex + offset) % kWorkerCount;

		if (m_workers[kCandidateIndex].localQueue.tryPop(handle)) {
			return true;
		}
	}

	return false;
}

void TaskSystem::enqueueReadyTask(TaskHandle handle, TaskAffinity affinity, TaskPriority priority) noexcept {
	TRIVIAL_ASSERT(handle.isValid());
	TRIVIAL_ASSERT(priority < TaskPriority::Count);

	m_affinityQueues[static_cast<std::size_t>(affinity)].enqueue(handle, priority);

	if (affinity == TaskAffinity::AnyWorker) {
		wakeOneIfUnderTarget();
	}
	// TODO: Switch for when having proper dedicated threads
}

void TaskSystem::completeTask(TaskHandle handle) noexcept {
	std::vector<TaskHandle> completionDependants; // TODO: Thread local vectors to reduce allocations
	m_graph.completeAndCollectDependants(handle, completionDependants);

	for (const TaskHandle kDependant : completionDependants) {
		TaskReadyInfo readyInfo{};

		if (m_graph.removePrerequisiteAndMarkReadyIfUnblocked(kDependant, handle, readyInfo)) {
			enqueueReadyTask(kDependant, readyInfo.affinity, readyInfo.priority);
		}
	}
}

bool TaskSystem::tryPopAndRunOneAnyWorkerTask() noexcept {
	constexpr std::size_t kAnyWorkerIndex = static_cast<std::size_t>(TaskAffinity::AnyWorker);

	TaskHandle handle{};

	if (!m_affinityQueues[kAnyWorkerIndex].tryPop(handle)) {
		return false;
	}

	runAndCompleteClaimedTask(handle);

	return true;
}

void TaskSystem::runAndCompleteClaimedTask(TaskHandle handle) noexcept {
	TRIVIAL_PROFILE_SCOPE("Task execution");

	const TaskClaimResult kClaimResult = m_graph.tryClaim(handle);

	if (kClaimResult != TaskClaimResult::Success) {
		return;
	}

	m_graph.executeClaimed(handle);
	completeTask(handle);
}

bool TaskSystem::tryHelpComplete(TaskHandle target, TaskAffinity callerAffinity, std::uint32_t maxDepth) noexcept {
	struct StackEntry {
		TaskHandle handle;
		std::uint32_t depth;
	};

	std::vector<StackEntry> stack; // TODO: Custom allocator
	std::vector<TaskHandle> visited;

	std::vector<TaskHandle> prerequisitesScratch;

	stack.push_back({.handle = target, .depth = 0});

	while (!stack.empty()) {
		const StackEntry kEntry = stack.back();
		stack.pop_back();

		bool alreadyVisited = false;
		for (const TaskHandle kVisitedHandle : visited) {
			if (kVisitedHandle == kEntry.handle) {
				alreadyVisited = true;
				break;
			}
		}

		if (alreadyVisited) {
			continue;
		}

		visited.push_back(kEntry.handle);

		TaskWalkInfo info{};

		if (!m_graph.tryGetWalkInfo(kEntry.handle, info, prerequisitesScratch)) {
			continue;
		}

		if (info.status == TaskStatus::Ready) {
			if (info.affinity == callerAffinity) {
				runAndCompleteClaimedTask(kEntry.handle);
			}

			continue;
		}

		if (info.status == TaskStatus::Waiting) {
			if (kEntry.depth < maxDepth) {
				for (const TaskHandle kPrerequisite : prerequisitesScratch) {
					stack.push_back({.handle = kPrerequisite, .depth = kEntry.depth + 1});
				}
			}

			continue;
		}
	}

	return isComplete(target);
}

} // namespace trivial::task

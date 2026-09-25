#include <trivial/task/task_system.h>

#include <cstdint>
#include <cstdlib>
#include <string>
#include <utility>

#include <trivial/core/assert.h>
#include <trivial/core/log.h>
#include <trivial/core/profile.h>
#include <trivial/core/sync/lock_guard.h>

#include "core/sync/parking_lot.h"

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

[[nodiscard]] std::uintptr_t keyFor(const trivial::task::Worker& worker) noexcept {
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	return reinterpret_cast<std::uintptr_t>(&worker);
}

} // namespace

namespace trivial::task {

TaskSystem::TaskSystem(const TaskSystemConfig& config)
    : m_affinityQueues{TaskPriorityQueue(config.scheduler), TaskPriorityQueue()}
    , m_targetActiveWorkerCount(static_cast<std::size_t>(config.workers.count))
    , m_activeSlots(m_targetActiveWorkerCount)
    , m_waitHelpMaxDepth(config.waitHelpMaxDepth) {
	TRIVIAL_ASSERT(config.workers.count > 0);

	const std::size_t kWorkerCount
	    = m_targetActiveWorkerCount + static_cast<std::size_t>(config.workers.maxStandbyWorkers);

	for (std::size_t i = 0; i < kWorkerCount; ++i) {
		m_workers.emplace_back();
	}

	thread::ThreadConfig threadConfig{
	    .name = "Worker",
	    .type = thread::ThreadType::Worker,
#if TRIVIAL_PLATFORM_POSIX
	    .stackAllocator = &m_workerStackAllocator,
#endif // TRIVIAL_PLATFORM_POSIX
	};

	for (Worker& worker : m_workers) {
		const thread::ThreadCreateResult kResult
		    = worker.thread.create(threadConfig, &TaskSystem::workerThreadEntry, static_cast<void*>(this));

		if (kResult.error != thread::ThreadCreateError::None) [[unlikely]] {
			TRIVIAL_LOG_FATAL_PREFIX("TaskSystem", "Failed to create worker thread");
			std::abort();
		}
	}
}

TaskSystem::~TaskSystem() noexcept {
	constexpr auto kAnyWorkerIndex = static_cast<std::size_t>(TaskAffinity::AnyWorker);
	constexpr auto kMainThreadIndex = static_cast<std::size_t>(TaskAffinity::MainThread);

	for (;;) {
		runMainThreadReadyTasks();

		while (tryPopAndRunOneAnyWorkerTask()) {}

		bool allParked = true;

		for (const Worker& worker : m_workers) {
			if (worker.state.load(std::memory_order_acquire) != WorkerState::Parked) {
				allParked = false;
			}
		}

		const bool kInjectionQueuesEmpty
		    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
		    = m_affinityQueues[kAnyWorkerIndex].empty() && m_affinityQueues[kMainThreadIndex].empty();

		if (allParked && kInjectionQueuesEmpty) {
			break;
		}
	}

#if TRIVIAL_ENABLE_ASSERTS
	for (const Worker& worker : m_workers) {
		TRIVIAL_ASSERT(worker.localQueue.empty());
	}
#endif // TRIVIAL_ENABLE_ASSERTS

	for (Worker& worker : m_workers) {
		worker.stopping.store(true, std::memory_order_relaxed);
		(void)sync::activeParkingLot().unparkOne(keyFor(worker));
	}

	for (Worker& worker : m_workers) {
		if (worker.thread.joinable()) {
			worker.thread.join();
		}
	}
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

	if (kWorkerIndex == kInvalidWorkerIndex) {
		latch.wait();
		return;
	}

	Worker& worker = m_workers[kWorkerIndex]; // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

	worker.state.store(WorkerState::Waiting, std::memory_order_relaxed);

	m_activeSlots.release();

	wakeOneIfUnderTarget();

	latch.wait();

	m_activeSlots.acquire();

	worker.state.store(WorkerState::Active, std::memory_order_relaxed);
}

void TaskSystem::wait(std::span<const TaskHandle> tasks) noexcept {
	bool allComplete = true;

	for (TaskHandle task : tasks) {
		TRIVIAL_ASSERT(task.isValid());

		if (isComplete(task)) {
			continue;
		}

		if (tryHelpComplete(task, TaskAffinity::AnyWorker, m_waitHelpMaxDepth)) {
			continue;
		}

		allComplete = false;
	}

	if (allComplete) {
		return;
	}

	// Extra slot to account for race of tasks finshing before all waiters area attached
	sync::Latch latch{tasks.size() + 1};

	for (TaskHandle task : tasks) {
		if (m_graph.tryAttachWaiter(task, latch) == TaskAttachWaiterResult::AlreadyComplete) {
			latch.countDown();
		}
	}

	latch.countDown(); // release the phantom slot

	const std::size_t kWorkerIndex = tryGetCurrentWorkerIndex();

	if (kWorkerIndex == kInvalidWorkerIndex) {
		latch.wait();
		return;
	}

	Worker& worker = m_workers[kWorkerIndex]; // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

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
	constexpr auto kMainThreadIndex = static_cast<std::size_t>(TaskAffinity::MainThread);

	TaskHandle handle{};

	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
	while (m_affinityQueues[kMainThreadIndex].tryPop(handle)) {
		runAndCompleteClaimedTask(handle);
	}
}

std::size_t TaskSystem::tryGetCurrentWorkerIndex() const noexcept {
	if (g_currentWorker == nullptr) {
		return kInvalidWorkerIndex;
	}

	TRIVIAL_ASSERT(g_currentWorkerSystem == this);

	return g_currentWorker->index;
}

void TaskSystem::workerThreadEntry(void* arg) noexcept {
	TaskSystem* system = static_cast<TaskSystem*>(arg);
	const std::size_t kIndex = system->m_nextWorkerStartIndex.fetch_add(1, std::memory_order_relaxed);

	Worker& worker = system->m_workers[kIndex]; // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
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
	Worker& worker = m_workers[workerIndex]; // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

#if TRIVIAL_ENABLE_TRACY
	TRIVIAL_PROFILE_THREAD(thread::Thread::current()->name());
#endif // TRIVIAL_ENABLE_TRACY

	bool holdingSlot = m_activeSlots.tryAcquire();

	while (!worker.stopping.load(std::memory_order_relaxed)) {
		TaskHandle handle{};

		if (worker.localQueue.tryPop(handle)) {
			runAndCompleteClaimedTask(handle);
			continue;
		}

		if (holdingSlot) {
			constexpr auto kAnyWorkerIndex = static_cast<std::size_t>(TaskAffinity::AnyWorker);

			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
			const std::size_t kGranted = m_affinityQueues[kAnyWorkerIndex].tryPopWeightedBatchInto(worker.localQueue);
			if (kGranted > 0 && worker.localQueue.tryPop(handle)) {
				runAndCompleteClaimedTask(handle);
				continue;
			}

			if (tryStealTask(workerIndex, handle)) {
				runAndCompleteClaimedTask(handle);
				continue;
			}

			m_activeSlots.release();
			holdingSlot = false;
		}

		if (!parkWorker(workerIndex)) {
			break;
		}

		holdingSlot = true;
	}

	if (holdingSlot) {
		m_activeSlots.release();
	}
}

bool TaskSystem::parkWorker(std::size_t workerIndex) noexcept {
	Worker& worker = m_workers[workerIndex]; // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

	worker.state.store(WorkerState::Parked, std::memory_order_relaxed);

	{
		sync::LockGuard<sync::Mutex> parkedLock(m_parkedIndicesMutex);
		m_parkedWorkerIndices.push_back(workerIndex);
	}

	constexpr auto kAnyWorkerIndex = static_cast<std::size_t>(TaskAffinity::AnyWorker);

	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
	if (!m_affinityQueues[kAnyWorkerIndex].empty() && m_activeSlots.tryAcquire()) {
		removeParkedIndex(workerIndex);
		worker.state.store(WorkerState::Active, std::memory_order_relaxed);
		return true;
	}

	(void)sync::activeParkingLot().park(keyFor(worker), [&worker] {
		return worker.state.load(std::memory_order_acquire) != WorkerState::Active
		       && !worker.stopping.load(std::memory_order_relaxed);
	});

	return !worker.stopping.load(std::memory_order_relaxed);
}

void TaskSystem::wakeWorker(std::size_t workerIndex) noexcept {
	Worker& worker = m_workers[workerIndex]; // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

	worker.state.store(WorkerState::Active, std::memory_order_release);
	(void)sync::activeParkingLot().unparkOne(keyFor(worker));
}

void TaskSystem::wakeOneIfUnderTarget() noexcept {
	if (!m_activeSlots.tryAcquire()) {
		return;
	}

	std::size_t indexToWake = 0;
	bool foundParked = false;

	{
		sync::LockGuard<sync::Mutex> parkedLock(m_parkedIndicesMutex);

		if (!m_parkedWorkerIndices.empty()) {
			indexToWake = m_parkedWorkerIndices.back();
			m_parkedWorkerIndices.pop_back();
			foundParked = true;
		}
	}

	if (!foundParked) {
		m_activeSlots.release();
		return;
	}

	wakeWorker(indexToWake);
}

void TaskSystem::removeParkedIndex(std::size_t workerIndex) noexcept {
	sync::LockGuard<sync::Mutex> parkedLock(m_parkedIndicesMutex);

	for (std::size_t i = 0; i < m_parkedWorkerIndices.size(); ++i) {
		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index, cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
		if (m_parkedWorkerIndices[i] == workerIndex) {
			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index, cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
			m_parkedWorkerIndices[i] = m_parkedWorkerIndices.back();
			m_parkedWorkerIndices.pop_back();
			return;
		}
	}
}

bool TaskSystem::tryStealTask(std::size_t workerIndex, TaskHandle& handle) noexcept {
	const std::size_t kWorkerCount = m_workers.size();

	if (kWorkerCount <= 1) {
		return false;
	}

	// TODO: More even load balancing whilst staying light weight - custom hash?
	for (std::size_t offset = 1; offset < kWorkerCount; ++offset) {
		const std::size_t kCandidateIndex = (workerIndex + offset) % kWorkerCount;

		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
		if (m_workers[kCandidateIndex].localQueue.tryPop(handle)) {
			return true;
		}
	}

	return false;
}

void TaskSystem::enqueueReadyTask(TaskHandle handle, TaskAffinity affinity, TaskPriority priority) noexcept {
	TRIVIAL_ASSERT(handle.isValid());
	TRIVIAL_ASSERT(priority < TaskPriority::Count);

	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index, cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
	m_affinityQueues[static_cast<std::size_t>(affinity)].enqueue(handle, priority);

	if (affinity == TaskAffinity::AnyWorker) {
		wakeOneIfUnderTarget();
	}
	// TODO: Switch for when having proper dedicated threads
}

void TaskSystem::completeTask(TaskHandle handle) noexcept {
	std::vector<TaskHandle> completionDependants; // TODO: Thread local vectors to reduce allocations
	m_graph.completeAndCollectDependants(handle, completionDependants);

	for (TaskHandle dependant : completionDependants) {
		TaskReadyInfo readyInfo{};

		if (m_graph.removePrerequisiteAndMarkReadyIfUnblocked(dependant, handle, readyInfo)) {
			enqueueReadyTask(dependant, readyInfo.affinity, readyInfo.priority);
		}
	}
}

bool TaskSystem::tryPopAndRunOneAnyWorkerTask() noexcept {
	constexpr auto kAnyWorkerIndex = static_cast<std::size_t>(TaskAffinity::AnyWorker);

	TaskHandle handle{};

	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
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
		for (TaskHandle visitedHandle : visited) {
			if (visitedHandle == kEntry.handle) {
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
				for (TaskHandle prerequisite : prerequisitesScratch) {
					stack.push_back({.handle = prerequisite, .depth = kEntry.depth + 1});
				}
			}

			continue;
		}
	}

	return isComplete(target);
}

} // namespace trivial::task

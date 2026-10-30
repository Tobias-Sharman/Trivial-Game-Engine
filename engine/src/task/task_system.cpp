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

#define TRIVIAL_TASK_SYSTEM_INVALID_WORKER_ID (std::numeric_limits<std::size_t>::max())

namespace {

// Could do some arithmetic operation on current thread - offset, where offset
// is the number of designated threads, but in order to protect against thread
// creation not on the main thread in the future (same vein as the given thread
// index being atomic) the cost of the memory for a handful of pointers is
// insignificant. Has the same indirection as going through thread index too so
// gain would only be memory related
//
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
constinit thread_local trivial::task::Worker* g_currentWorker = nullptr;

#if TRIVIAL_ENABLE_ASSERTS
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
constinit thread_local trivial::task::TaskSystem* g_currentWorkerSystem = nullptr;
#endif // TRIVIAL_ENABLE_ASSERTS

} // namespace

namespace trivial::task {

TaskSystem::TaskSystem(const TaskSystemConfig& config) noexcept
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
	TRIVIAL_ASSERT(m_parkedWorkerIds.empty());
#endif // TRIVIAL_ENABLE_ASSERTS
}

TaskHandle TaskSystem::launch(TaskPayload payload, const TaskLaunchOptions& options) noexcept {
	const TaskCreateDispatchOutcome kOutcome = m_graph.createDispatched(std::move(payload), {}, options);

	if (kOutcome.createResult != TaskCreateResult::Success) [[unlikely]] {
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

	if (kOutcome.createResult != TaskCreateResult::Success) [[unlikely]] {
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

	if (kOutcome.createResult != TaskCreateResult::Success) [[unlikely]] {
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

	const std::size_t kWorkerId = tryGetCurrentWorkerId();

	if (kWorkerId == TRIVIAL_TASK_SYSTEM_INVALID_WORKER_ID) {
		latch.wait();
		return;
	}

	Worker& worker = m_workers[kWorkerId];

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

	const std::size_t kWorkerId = tryGetCurrentWorkerId();

	if (kWorkerId == TRIVIAL_TASK_SYSTEM_INVALID_WORKER_ID) {
		latch.wait();
		return;
	}

	Worker& worker = m_workers[kWorkerId];

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
	constexpr std::size_t kMainThreadQueue = static_cast<std::size_t>(TaskAffinity::MainThread);

	TaskHandle handle{};

	while (m_affinityQueues[kMainThreadQueue].tryPop(handle)) {
		runAndCompleteClaimedTask(handle);
	}
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
std::size_t TaskSystem::tryGetCurrentWorkerId() const noexcept {
	if (g_currentWorker == nullptr) {
		return TRIVIAL_TASK_SYSTEM_INVALID_WORKER_ID;
	}

	TRIVIAL_ASSERT(g_currentWorkerSystem == this);

	return g_currentWorker->id;
}

void TaskSystem::workerThreadEntry(void* arg) noexcept {
	TaskSystem* system = static_cast<TaskSystem*>(arg);
	const std::size_t kWorkerId = system->m_nextWorkerId.fetch_add(1, std::memory_order_relaxed);

	Worker& worker = system->m_workers[kWorkerId];
	worker.id = kWorkerId;

	const std::string kThreadName = "Worker " + std::to_string(kWorkerId);
	thread::Thread::current()->rename(kThreadName.c_str());

	g_currentWorker = &worker;
#if TRIVIAL_ENABLE_ASSERTS
	g_currentWorkerSystem = system;
#endif // TRIVIAL_ENABLE_ASSERTS

	system->runWorkerLoop(kWorkerId);
}

void TaskSystem::runWorkerLoop(std::size_t workerId) noexcept {
	Worker& worker = m_workers[workerId];

	TRIVIAL_PROFILE_THREAD(thread::Thread::current()->name());

	bool holdingSlot = m_activeSlots.tryAcquire();

	for (;;) {
		TaskHandle handle{};

		if (worker.localQueue.tryPop(handle)) {
			runAndCompleteClaimedTask(handle);
			continue;
		}

		if (holdingSlot && tryClaimSharedTask(workerId, handle)) {
			runAndCompleteClaimedTask(handle);
			continue;
		}

		if (worker.stopping.load(std::memory_order_acquire)) {
			break;
		}

		if (holdingSlot) {
			m_activeSlots.release();
		}

		holdingSlot = parkWorker(workerId);
	}

	if (holdingSlot) {
		m_activeSlots.release();
	}
}

bool TaskSystem::parkWorker(std::size_t workerId) noexcept {
	Worker& worker = m_workers[workerId];

	worker.state.store(WorkerState::Parked, std::memory_order_relaxed);

	{
		const sync::LockGuard<sync::Mutex> kParkedLock(m_parkedIndicesMutex);
		m_parkedWorkerIds.push_back(workerId);
	}

	constexpr std::size_t kAnyWorkerQueue = static_cast<std::size_t>(TaskAffinity::AnyWorker);

	if (!m_affinityQueues[kAnyWorkerQueue].empty() && m_activeSlots.tryAcquire()) {
		if (!tryRemoveParkedWorker(workerId)) {
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

	return !tryRemoveParkedWorker(workerId);
}

void TaskSystem::wakeOneIfUnderTarget() noexcept {
	if (!m_activeSlots.tryAcquire()) {
		return;
	}

	Worker* workerToWake = nullptr;

	{
		const sync::LockGuard<sync::Mutex> kParkedLock(m_parkedIndicesMutex);

		if (!m_parkedWorkerIds.empty()) {
			workerToWake = &m_workers[m_parkedWorkerIds.back()];
			m_parkedWorkerIds.pop_back();
			workerToWake->state.store(WorkerState::Active, std::memory_order_release);
		}
	}

	if (workerToWake == nullptr) {
		m_activeSlots.release();
		return;
	}

	(void)sync::activeParkingLot().unparkOne(sync::parkingKey(*workerToWake));
}

bool TaskSystem::tryRemoveParkedWorker(std::size_t workerId) noexcept {
	const sync::LockGuard<sync::Mutex> kParkedLock(m_parkedIndicesMutex);

	for (std::size_t i = 0; i < m_parkedWorkerIds.size(); ++i) {
		if (m_parkedWorkerIds[i] == workerId) {
			m_parkedWorkerIds[i] = m_parkedWorkerIds.back();
			m_parkedWorkerIds.pop_back();
			return true;
		}
	}

	return false;
}

bool TaskSystem::tryClaimSharedTask(std::size_t workerId, TaskHandle& handle) noexcept {
	Worker& worker = m_workers[workerId];

	constexpr std::size_t kAnyWorkerQueue = static_cast<std::size_t>(TaskAffinity::AnyWorker);

	const std::size_t kGranted = m_affinityQueues[kAnyWorkerQueue].tryPopWeightedBatchInto(worker.localQueue);
	if (kGranted > 0 && worker.localQueue.tryPop(handle)) {
		return true;
	}

	return tryStealTask(workerId, handle);
}

bool TaskSystem::tryStealTask(std::size_t workerId, TaskHandle& handle) noexcept {
	const std::size_t kWorkerCount = m_workers.size();

	if (kWorkerCount <= 1) {
		return false;
	}

	for (std::size_t offset = 1; offset < kWorkerCount; ++offset) {
		const std::size_t kCandidateId = (workerId + offset) % kWorkerCount;

		if (m_workers[kCandidateId].localQueue.tryPop(handle)) {
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
}

void TaskSystem::completeTask(TaskHandle handle) noexcept {
	std::vector<TaskHandle> completionDependants;
	m_graph.completeAndCollectDependants(handle, completionDependants);

	for (const TaskHandle kDependant : completionDependants) {
		TaskReadyInfo readyInfo{};

		if (m_graph.removePrerequisiteAndMarkReadyIfUnblocked(kDependant, handle, readyInfo)) {
			enqueueReadyTask(kDependant, readyInfo.affinity, readyInfo.priority);
		}
	}
}

bool TaskSystem::tryPopAndRunOneAnyWorkerTask() noexcept {
	constexpr std::size_t kAnyWorkerQueue = static_cast<std::size_t>(TaskAffinity::AnyWorker);

	TaskHandle handle{};

	if (!m_affinityQueues[kAnyWorkerQueue].tryPop(handle)) {
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

	std::vector<StackEntry> stack;
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

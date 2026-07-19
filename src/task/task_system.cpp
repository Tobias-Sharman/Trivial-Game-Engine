#include <trivial/task/task_system.h>

#include <mutex>
#include <utility>

#include <trivial/core/assert.h>
#include <trivial/core/profile.h>

namespace {

std::uint32_t resolveWorkerCount(uint32_t requested) {
	if (requested == 0) {
		auto hardware = std::thread::hardware_concurrency();

		if (hardware == 0U) {
			TRIVIAL_LOG_ERROR("Could not resolve hardware concurrency");

			return 1;
		}

		return hardware;
	}

	return requested;
}

} // namespace

namespace trivial::task {

TaskSystem::TaskSystem(const TaskSystemConfig& config)
    : m_affinityQueues{TaskPriorityQueue(config.scheduler), TaskPriorityQueue()}
    , m_targetActiveWorkerCount(static_cast<std::size_t>(resolveWorkerCount(config.workers.count)))
    , m_activeSlots(static_cast<std::ptrdiff_t>(m_targetActiveWorkerCount))
    , m_waitHelpMaxDepth(config.waitHelpMaxDepth) {
	const std::size_t kWorkerCount
	    = m_targetActiveWorkerCount + static_cast<std::size_t>(config.workers.maxStandbyWorkers);

	for (std::size_t i = 0; i < kWorkerCount; ++i) {
		m_workers.emplace_back(i, config.workers.thread);
	}

	for (Worker& worker : m_workers) {
		worker.thread = std::thread(&TaskSystem::runWorkerLoop, this, worker.index, worker.stopSource.get_token());
	}
}

TaskSystem::~TaskSystem() noexcept {
	constexpr auto kAnyWorkerIndex = static_cast<std::size_t>(TaskAffinity::AnyWorker);
	constexpr auto kMainThreadIndex = static_cast<std::size_t>(TaskAffinity::MainThread);

	while (true) {
		runMainThreadReadyTasks();

		while (tryPopAndRunOneAnyWorkerTask()) {}

		bool allParked = true;

		for (Worker& worker : m_workers) {
			std::lock_guard<std::mutex> stateLock(worker.stateMutex);

			if (worker.state != WorkerState::Parked) {
				allParked = false;
			}
		}

		const bool kInjectionQueuesEmpty
		    = m_affinityQueues[kAnyWorkerIndex].empty() && m_affinityQueues[kMainThreadIndex].empty();

		if (allParked && kInjectionQueuesEmpty) {
			break;
		}
	}

	for (const Worker& worker : m_workers) {
		TRIVIAL_ASSERT(worker.localQueue.empty());
	}

	for (Worker& worker : m_workers) {
		worker.stopSource.request_stop();
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

	TaskWaitGroup waitGroup{1};

	const TaskAttachWaiterResult kAttachResult = m_graph.tryAttachWaiter(task, waitGroup);

	if (kAttachResult == TaskAttachWaiterResult::AlreadyComplete) {
		return;
	}

	TRIVIAL_ASSERT(kAttachResult == TaskAttachWaiterResult::Attached);

	const std::size_t kWorkerIndex = tryGetCurrentWorkerIndex();

	if (kWorkerIndex == kInvalidWorkerIndex) {
		waitGroup.wait();
		return;
	}

	Worker& worker = m_workers[kWorkerIndex];

	{
		std::lock_guard<std::mutex> stateLock(worker.stateMutex);
		worker.state = WorkerState::Waiting;
	}

	m_activeSlots.release();

	wakeOneIfUnderTarget();

	const bool kSignaled = waitGroup.wait(worker.stopSource.get_token());

	m_activeSlots.acquire();

	{
		std::lock_guard<std::mutex> stateLock(worker.stateMutex);
		worker.state = WorkerState::Active;
	}

	if (!kSignaled) {
		m_graph.detachWaiterIfUnclaimed(task, waitGroup);
	}
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
	TaskWaitGroup waitGroup{tasks.size() + 1};

	for (TaskHandle task : tasks) {
		if (m_graph.tryAttachWaiter(task, waitGroup) == TaskAttachWaiterResult::AlreadyComplete) {
			TaskWaitGroup::onMemberComplete(&waitGroup);
		}
	}

	TaskWaitGroup::onMemberComplete(&waitGroup); // release the phantom slot

	const std::size_t kWorkerIndex = tryGetCurrentWorkerIndex();

	if (kWorkerIndex == kInvalidWorkerIndex) {
		waitGroup.wait();
		return;
	}

	Worker& worker = m_workers[kWorkerIndex];

	{
		std::lock_guard<std::mutex> stateLock(worker.stateMutex);
		worker.state = WorkerState::Waiting;
	}

	m_activeSlots.release();

	wakeOneIfUnderTarget();

	const bool kSignaled = waitGroup.wait(worker.stopSource.get_token());

	m_activeSlots.acquire();

	{
		std::lock_guard<std::mutex> stateLock(worker.stateMutex);
		worker.state = WorkerState::Active;
	}

	if (!kSignaled) {
		for (TaskHandle task : tasks) {
			if (!isComplete(task)) {
				m_graph.detachWaiterIfUnclaimed(task, waitGroup);
			}
		}
	}
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

	while (m_affinityQueues[kMainThreadIndex].tryPop(handle)) {
		runAndCompleteClaimedTask(handle);
	}
}

std::size_t TaskSystem::tryGetCurrentWorkerIndex() const noexcept {
	const std::thread::id kCurrentId = std::this_thread::get_id();

	for (std::size_t i = 0; i < m_workers.size(); ++i) {
		if (m_workers[i].thread.get_id() == kCurrentId) {
			return i;
		}
	}

	return kInvalidWorkerIndex;
}

void TaskSystem::runWorkerLoop(std::size_t workerIndex, const std::stop_token& stopToken) {
	Worker& worker = m_workers[workerIndex];

#if TRIVIAL_ENABLE_TRACY
	const std::string kThreadName = worker.config.name + " " + std::to_string(workerIndex);
	TRIVIAL_PROFILE_THREAD(kThreadName.c_str());
#endif // TRIVIAL_ENABLE_TRACY

	bool holdingSlot = m_activeSlots.try_acquire();

	while (!stopToken.stop_requested()) {
		TaskHandle handle{};

		if (worker.localQueue.tryPop(handle)) {
			runAndCompleteClaimedTask(handle);
			continue;
		}

		if (holdingSlot) {
			constexpr auto kAnyWorkerIndex = static_cast<std::size_t>(TaskAffinity::AnyWorker);

			m_affinityQueues[kAnyWorkerIndex].tryPopWeightedBatchInto(worker.localQueue);
			if (worker.localQueue.tryPop(handle)) {
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

		if (!parkWorker(workerIndex, stopToken)) {
			break;
		}

		holdingSlot = true;
	}

	if (holdingSlot) {
		m_activeSlots.release();
	}
}

bool TaskSystem::parkWorker(std::size_t workerIndex, const std::stop_token& stopToken) noexcept {
	Worker& worker = m_workers[workerIndex];

	std::unique_lock<std::mutex> stateLock(worker.stateMutex);

	worker.state = WorkerState::Parked;

	{
		std::lock_guard<TaskGraphMutex> parkedLock(m_parkedIndicesMutex);
		m_parkedWorkerIndices.push_back(workerIndex);
	}

	return worker.stateCv.wait(stateLock, stopToken, [&worker] {
		return worker.state == WorkerState::Active;
	});
}

void TaskSystem::wakeWorker(std::size_t workerIndex) noexcept {
	Worker& worker = m_workers[workerIndex];

	{
		std::lock_guard<std::mutex> stateLock(worker.stateMutex);
		worker.state = WorkerState::Active;
	}

	worker.stateCv.notify_one();
}

void TaskSystem::wakeOneIfUnderTarget() noexcept {
	if (!m_activeSlots.try_acquire()) {
		return;
	}

	std::size_t indexToWake = 0;
	bool foundParked = false;

	{
		std::lock_guard<TaskGraphMutex> parkedLock(m_parkedIndicesMutex);

		if (!m_parkedWorkerIndices.empty()) {
			indexToWake = m_parkedWorkerIndices.back();
			m_parkedWorkerIndices.pop_back();
			foundParked = true;
		}
	}

	if (!foundParked) {
		m_activeSlots.release(); // nobody to hand it to - give it back
		return;
	}

	wakeWorker(indexToWake);
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

	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
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

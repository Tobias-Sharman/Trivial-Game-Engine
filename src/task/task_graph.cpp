#include <trivial/task/task_graph.h>

#include <mutex>
#include <new>

#include <trivial/core/assert.h>
#include <trivial/core/log.h>

namespace trivial::task {

TaskGraph::~TaskGraph() noexcept {
	for (std::atomic<TaskPage*>& pageEntry : m_pages) {
		TaskPage* page = pageEntry.load(std::memory_order_relaxed);

#if TRIVIAL_CONFIG_DEBUG
		// NOTE: Only valid if no workers or tasks are running
		if (page != nullptr) {
			for (TaskSlot& slot : *page) {
				if (!slot.isOccupied()) {
					continue;
				}
				const TaskStatus kStatus = slot.state().status;
				TRIVIAL_ASSERT(kStatus != TaskStatus::Running);
				TRIVIAL_ASSERT(kStatus != TaskStatus::Completing);
			}
		}
#endif // TRIVIAL_CONFIG_DEBUG

		delete page;
	}
}

TaskCreateOutcome TaskGraph::create(TaskFunction function, const TaskLaunchOptions& options) noexcept {
	std::uint32_t taskIndex = 0;

	if (!allocateTaskIndex(taskIndex)) {
		return {.result = TaskCreateResult::CapacityExhausted, .handle = {}};
	}

	TaskPage* page = ensurePage(pageIndexFor(taskIndex));

	if (page == nullptr) {
		releaseTaskIndex(taskIndex);

		return {.result = TaskCreateResult::AllocationFailure, .handle = {}};
	}

	TaskSlot* slot = slotInPage(*page, taskIndex);

	std::lock_guard<TaskSlotMutex> lock(slot->mutex());

	TRIVIAL_ASSERT(!slot->isOccupied());

	slot->construct(std::move(function), options);

	return {.result = TaskCreateResult::Success,
	        .handle = TaskHandle{.index = taskIndex, .generation = slot->generation()}};
}

TaskPrerequisiteResult TaskGraph::addPrerequisite(TaskHandle taskHandle, TaskHandle prerequisiteHandle) noexcept {
	if (!taskHandle.isValid() || !prerequisiteHandle.isValid()) {
		return TaskPrerequisiteResult::InvalidHandle;
	}

	if (taskHandle == prerequisiteHandle) {
		return TaskPrerequisiteResult::SelfDependency;
	}

#if TRIVIAL_CONFIG_DEBUG
	std::lock_guard<TaskGraphMutex> topologyLock(m_debugTopologyMutex);

	if (wouldCreateCycle(taskHandle, prerequisiteHandle)) {
		TRIVIAL_LOG_ERROR("Task graph tried to create a dependency loop/cycle");

		return TaskPrerequisiteResult::InvalidState;
	}
#endif // TRIVIAL_CONFIG_DEBUG

	TaskSlot* dependantSlot = slotAt(taskHandle.index);
	TaskSlot* prerequisiteSlot = slotAt(prerequisiteHandle.index);

	if (dependantSlot == nullptr || prerequisiteSlot == nullptr) {
		return TaskPrerequisiteResult::InvalidHandle;
	}

	std::scoped_lock lock(dependantSlot->mutex(), prerequisiteSlot->mutex());

	if (!dependantSlot->isOccupiedBy(taskHandle) || !prerequisiteSlot->isOccupiedBy(prerequisiteHandle)) {
		return TaskPrerequisiteResult::InvalidHandle;
	}

	TaskState& dependantState = dependantSlot->state();
	TaskState& prerequisiteState = prerequisiteSlot->state();

	// Avoids a race that can end up in a task waiting forever
	if (dependantState.status != TaskStatus::Created) {
		return TaskPrerequisiteResult::InvalidState;
	}

	if (prerequisiteState.status == TaskStatus::Cancelled) {
		return TaskPrerequisiteResult::InvalidState;
	}

	for (const TaskHandle& existingPrerequisite : dependantState.prerequisites) {
		if (existingPrerequisite == prerequisiteHandle) {
			return TaskPrerequisiteResult::DuplicateDependency;
		}
	}

	if (prerequisiteState.status == TaskStatus::Completed) {
		return TaskPrerequisiteResult::Success;
	}

	dependantState.prerequisites.push_back(prerequisiteHandle);
	prerequisiteState.dependants.push_back(taskHandle);

	return TaskPrerequisiteResult::Success;
}

TaskDispatchOutcome TaskGraph::dispatch(TaskHandle handle) noexcept {
	if (!handle.isValid()) {
		return {.result = TaskDispatchResult::InvalidHandle, .readiness = TaskReadiness::Waiting, .priority = {}};
	}

	TaskSlot* slot = slotAt(handle.index);

	if (slot == nullptr) {
		return {.result = TaskDispatchResult::InvalidHandle, .readiness = TaskReadiness::Waiting, .priority = {}};
	}

	std::lock_guard<TaskSlotMutex> lock(slot->mutex());

	if (!slot->isOccupiedBy(handle)) {
		return {.result = TaskDispatchResult::InvalidHandle, .readiness = TaskReadiness::Waiting, .priority = {}};
	}

	TaskState& state = slot->state();

	if (state.status != TaskStatus::Created) {
		return {.result = TaskDispatchResult::AlreadyDispatched, .readiness = TaskReadiness::Waiting, .priority = {}};
	}

	if (state.prerequisites.empty()) {
		state.status = TaskStatus::Ready;

		return {.result = TaskDispatchResult::Success,
		        .readiness = TaskReadiness::Ready,
		        .priority = state.effectivePriority};
	}

	state.status = TaskStatus::Waiting;

	return {.result = TaskDispatchResult::Success,
	        .readiness = TaskReadiness::Waiting,
	        .priority = state.effectivePriority};
}

TaskClaimResult TaskGraph::tryClaim(TaskHandle handle) noexcept {
	if (!handle.isValid()) {
		return TaskClaimResult::InvalidHandle;
	}

	TaskSlot* slot = slotAt(handle.index);

	if (slot == nullptr) {
		return TaskClaimResult::InvalidHandle;
	}

	std::lock_guard<TaskSlotMutex> lock(slot->mutex());

	if (!slot->isOccupiedBy(handle)) {
		return TaskClaimResult::InvalidHandle;
	}

	TaskState& state = slot->state();

	if (state.status != TaskStatus::Ready) {
		return TaskClaimResult::NotReady;
	}

	state.status = TaskStatus::Running;

	return TaskClaimResult::Success;
}

void TaskGraph::executeClaimed(TaskHandle handle) noexcept {
	TRIVIAL_ASSERT(handle.isValid());

	TaskSlot* slot = slotAt(handle.index);

	TRIVIAL_ASSERT(slot != nullptr);

	TaskFunction* function = nullptr;

	{
		std::lock_guard<TaskSlotMutex> lock(slot->mutex());

		TRIVIAL_ASSERT(slot->isOccupiedBy(handle));

		TaskState& state = slot->state();

		TRIVIAL_ASSERT(state.status == TaskStatus::Running);

		function = &state.function;
	}

	TRIVIAL_ASSERT(function != nullptr);

	(*function)();
}

void TaskGraph::beginCompletion(TaskHandle handle, std::vector<TaskHandle>& outDependants) noexcept {
	TRIVIAL_ASSERT(handle.isValid());

	TaskSlot* slot = slotAt(handle.index);

	TRIVIAL_ASSERT(slot != nullptr);

	std::lock_guard<TaskSlotMutex> lock(slot->mutex());

	TRIVIAL_ASSERT(slot->isOccupiedBy(handle));

	TaskState& state = slot->state();

	TRIVIAL_ASSERT(state.status == TaskStatus::Running);

	state.status = TaskStatus::Completing;

	outDependants.clear();
	outDependants.insert(outDependants.end(), state.dependants.begin(), state.dependants.end());
}

bool TaskGraph::removePrerequisiteAndMarkReadyIfUnblocked(TaskHandle dependant,
                                                          TaskHandle prerequisite,
                                                          TaskPriority& readyPriority) noexcept {
	TRIVIAL_ASSERT(dependant.isValid());
	TRIVIAL_ASSERT(prerequisite.isValid());

	TaskSlot* dependantSlot = slotAt(dependant.index);

	TRIVIAL_ASSERT(dependantSlot != nullptr);

	std::lock_guard<TaskSlotMutex> lock(dependantSlot->mutex());

	TRIVIAL_ASSERT(dependantSlot->isOccupiedBy(dependant));

	TaskState& dependantState = dependantSlot->state();

	TRIVIAL_ASSERT(dependantState.status == TaskStatus::Waiting);

	std::vector<TaskHandle>& prerequisites = dependantState.prerequisites;

	std::size_t prerequisiteIndex = prerequisites.size();

	for (std::size_t i = 0; i < prerequisites.size(); ++i) {
		if (prerequisites[i] == prerequisite) {
			prerequisiteIndex = i;
			break;
		}
	}

	TRIVIAL_ASSERT(prerequisiteIndex != prerequisites.size());

	prerequisites[prerequisiteIndex] = prerequisites.back();
	prerequisites.pop_back();

	if (!prerequisites.empty()) {
		return false;
	}

	dependantState.status = TaskStatus::Ready;
	readyPriority = dependantState.effectivePriority;

	return true;
}

void TaskGraph::finishCompletion(TaskHandle handle) noexcept {
	TRIVIAL_ASSERT(handle.isValid());

	TaskSlot* slot = slotAt(handle.index);

	TRIVIAL_ASSERT(slot != nullptr);

	std::lock_guard<TaskSlotMutex> lock(slot->mutex());

	TRIVIAL_ASSERT(slot->isOccupiedBy(handle));

	TaskState& state = slot->state();

	TRIVIAL_ASSERT(state.status == TaskStatus::Completing);

	state.status = TaskStatus::Completed;
}

TaskReleaseResult TaskGraph::release(TaskHandle handle) noexcept {
	if (!handle.isValid()) {
		return TaskReleaseResult::InvalidHandle;
	}

	TaskSlot* slot = slotAt(handle.index);

	if (slot == nullptr) {
		return TaskReleaseResult::InvalidHandle;
	}

	{
		std::lock_guard<TaskSlotMutex> lock(slot->mutex());

		if (!slot->isOccupiedBy(handle)) {
			return TaskReleaseResult::InvalidHandle;
		}

		TaskState& state = slot->state();

		if (state.status != TaskStatus::Completed && state.status != TaskStatus::Cancelled) {
			return TaskReleaseResult::TaskNotComplete;
		}

		slot->destroy();
	}

	releaseTaskIndex(handle.index);

	return TaskReleaseResult::Success;
}

bool TaskGraph::tryGetStatus(TaskHandle handle, TaskStatus& status) const noexcept {
	if (!handle.isValid()) {
		return false;
	}

	const TaskSlot* slot = slotAt(handle.index);

	if (slot == nullptr) {
		return false;
	}

	std::lock_guard<TaskSlotMutex> lock(slot->mutex());

	if (!slot->isOccupiedBy(handle)) {
		return false;
	}

	status = slot->state().status;

	return true;
}

TaskGraph::TaskPage* TaskGraph::pageAt(std::uint32_t pageIndex) noexcept {
	if (pageIndex >= kMaxPageCount) {
		return nullptr;
	}

	return m_pages[pageIndex].load(std::memory_order_acquire);
}

const TaskGraph::TaskPage* TaskGraph::pageAt(std::uint32_t pageIndex) const noexcept {
	if (pageIndex >= kMaxPageCount) {
		return nullptr;
	}

	return m_pages[pageIndex].load(std::memory_order_acquire);
}

TaskGraph::TaskPage* TaskGraph::ensurePage(std::uint32_t pageIndex) noexcept {
	if (pageIndex >= kMaxPageCount) {
		return nullptr;
	}

	TaskPage* page = pageAt(pageIndex);

	if (page != nullptr) {
		return page;
	}

	std::lock_guard<TaskGraphMutex> lock(m_pageCreationMutex);

	page = m_pages[pageIndex].load(std::memory_order_relaxed);

	if (page != nullptr) {
		return page;
	}

	TaskPage* newPage = new (std::nothrow) TaskPage;

	if (newPage == nullptr) {
		TRIVIAL_LOG_ERROR("TaskGraph failed to allocate task page");
		return nullptr;
	}

	m_pages[pageIndex].store(newPage, std::memory_order_release);

	return newPage;
}

TaskSlot* TaskGraph::slotAt(std::uint32_t taskIndex) noexcept {
	if (taskIndex >= kMaxTaskCount) {
		return nullptr;
	}

	TaskPage* page = pageAt(pageIndexFor(taskIndex));

	if (page == nullptr) {
		return nullptr;
	}

	return slotInPage(*page, taskIndex);
}

const TaskSlot* TaskGraph::slotAt(std::uint32_t taskIndex) const noexcept {
	if (taskIndex >= kMaxTaskCount) {
		return nullptr;
	}

	const TaskPage* page = pageAt(pageIndexFor(taskIndex));

	if (page == nullptr) {
		return nullptr;
	}

	return slotInPage(*page, taskIndex);
}

TaskSlot* TaskGraph::slotInPage(TaskPage& page, std::uint32_t taskIndex) noexcept {
	return &page[slotIndexFor(taskIndex)];
}

const TaskSlot* TaskGraph::slotInPage(const TaskPage& page, std::uint32_t taskIndex) noexcept {
	return &page[slotIndexFor(taskIndex)];
}

bool TaskGraph::allocateTaskIndex(std::uint32_t& taskIndex) noexcept {
	std::lock_guard<TaskGraphMutex> lock(m_allocationMutex);

	if (!m_freeTaskIndices.empty()) {
		taskIndex = m_freeTaskIndices.back();
		m_freeTaskIndices.pop_back();
		return true;
	}

	if (m_nextUnusedTaskIndex >= kMaxTaskCount) {
		TRIVIAL_LOG_WARNING("TaskGraph capacity exhausted you should increase the max task count");
		return false;
	}

	taskIndex = m_nextUnusedTaskIndex;
	++m_nextUnusedTaskIndex;

	return true;
}

void TaskGraph::releaseTaskIndex(std::uint32_t taskIndex) noexcept {
	TRIVIAL_ASSERT(taskIndex < kMaxTaskCount);

	std::lock_guard<TaskGraphMutex> lock(m_allocationMutex);

	m_freeTaskIndices.push_back(taskIndex); // TODO: Custom allocator
}

#if TRIVIAL_CONFIG_DEBUG

bool TaskGraph::wouldCreateCycle(TaskHandle taskHandle, TaskHandle prerequisiteHandle) const noexcept {
	TRIVIAL_ASSERT(taskHandle.isValid());
	TRIVIAL_ASSERT(prerequisiteHandle.isValid());
	TRIVIAL_ASSERT(taskHandle != prerequisiteHandle);

	std::vector<TaskHandle> dependantStack;

	dependantStack.push_back(taskHandle);

	while (!dependantStack.empty()) {
		const TaskHandle kCurrentHandle = dependantStack.back();
		dependantStack.pop_back();

		if (kCurrentHandle == prerequisiteHandle) {
			return true;
		}

		const TaskSlot* slot = slotAt(kCurrentHandle.index);

		TRIVIAL_ASSERT(slot != nullptr);

		std::lock_guard<TaskSlotMutex> lock(slot->mutex());

		TRIVIAL_ASSERT(slot->isOccupiedBy(kCurrentHandle));

		const TaskState& state = slot->state();

		for (const TaskHandle& dependantHandle : state.dependants) {
			dependantStack.push_back(dependantHandle);
		}
	}

	return false;
}

#endif
} // namespace trivial::task

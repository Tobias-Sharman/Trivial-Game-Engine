#include <trivial/task/task_graph.h>

#include <mutex>
#include <new>

#include <trivial/core/assert.h>
#include <trivial/core/log.h>

namespace trivial::task {

TaskGraph::~TaskGraph() noexcept { // NOLINT(readability-function-cognitive-complexity)
	for (std::atomic<TaskPage*>& pageEntry : m_pages) {
		TaskPage* page = pageEntry.load(std::memory_order_relaxed);

#if TRIVIAL_CONFIG_DEBUG
		// NOTE: Only valid if no workers or tasks are running
		if (page != nullptr) {
			for (TaskSlot& slot : *page) {
				if (!slot.isOccupied()) {
					continue;
				}

				const TaskStatus kStatus = slot.state().status();

				TRIVIAL_ASSERT(kStatus != TaskStatus::Running);
			}
		}
#endif // TRIVIAL_CONFIG_DEBUG

		delete page;
	}
}

TaskCreateDispatchOutcome TaskGraph::createDispatched(TaskPayload payload,
                                                      std::span<const TaskHandle> prerequisites,
                                                      const TaskLaunchOptions& options) noexcept {
	std::uint32_t taskIndex = 0;

	if (!allocateTaskIndex(taskIndex)) {
		return {.createResult = TaskCreateResult::CapacityExhausted};
	}

	TaskPage* page = ensurePage(pageIndexFor(taskIndex));

	if (page == nullptr) {
		releaseTaskIndex(taskIndex);
		return {.createResult = TaskCreateResult::AllocationFailure};
	}

	TaskSlot* slot = slotInPage(*page, taskIndex);

	std::lock_guard<TaskSlotMutex> lock(slot->mutex());

	TRIVIAL_ASSERT(!slot->isOccupied());

	TaskState& state = slot->construct(std::move(payload), options);

	const TaskHandle kHandle{.index = taskIndex, .generation = slot->generation()};

	for (TaskHandle prerequisite : prerequisites) {
		const TaskPrerequisiteResult kResult = addPrerequisiteLocked(kHandle, *slot, prerequisite);

		// NOLINTNEXTLINE(readability-simplify-boolean-expr)
		TRIVIAL_ASSERT(kResult == TaskPrerequisiteResult::Success
		               || kResult == TaskPrerequisiteResult::DuplicateDependency);
	}

	if (state.prerequisites.empty()) {
		state.setStatus(TaskStatus::Ready);
		return {.createResult = TaskCreateResult::Success,
		        .handle = kHandle,
		        .readiness = TaskReadiness::Ready,
		        .priority = state.priority()};
	}

	state.setStatus(TaskStatus::Waiting);
	return {.createResult = TaskCreateResult::Success,
	        .handle = kHandle,
	        .readiness = TaskReadiness::Waiting,
	        .priority = state.priority()};
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

	if (state.status() != TaskStatus::Ready) {
		return TaskClaimResult::NotReady;
	}

	state.setStatus(TaskStatus::Running);

	return TaskClaimResult::Success;
}

TaskAttachWaiterResult TaskGraph::tryAttachWaiter(TaskHandle handle, TaskWaitGroup& waitGroup) noexcept {
	if (!handle.isValid()) {
		return TaskAttachWaiterResult::InvalidHandle;
	}

	TaskSlot* slot = slotAt(handle.index);

	if (slot == nullptr) {
		return TaskAttachWaiterResult::InvalidHandle;
	}

	std::lock_guard<TaskSlotMutex> lock(slot->mutex());

	if (!slot->isOccupiedBy(handle)) {
		return TaskAttachWaiterResult::InvalidHandle;
	}

	TaskState& state = slot->state();

	if (state.status() == TaskStatus::Completed || state.status() == TaskStatus::Cancelled) {
		return TaskAttachWaiterResult::AlreadyComplete;
	}

	TRIVIAL_ASSERT(state.waitGroup == nullptr);

	state.waitGroup = &waitGroup;

	return TaskAttachWaiterResult::Attached;
}

void TaskGraph::executeClaimed(TaskHandle handle) noexcept {
	TRIVIAL_ASSERT(handle.isValid());

	TaskSlot* slot = slotAt(handle.index);

	TRIVIAL_ASSERT(slot != nullptr);

	TaskPayload* payload = nullptr;

	{
		std::lock_guard<TaskSlotMutex> lock(slot->mutex());

		TRIVIAL_ASSERT(slot->isOccupiedBy(handle));

		TaskState& state = slot->state();

		TRIVIAL_ASSERT(state.status() == TaskStatus::Running);

		payload = &state.payload;
	}

	TRIVIAL_ASSERT(payload != nullptr);

	(*payload)();
}

void TaskGraph::completeAndCollectDependants(TaskHandle handle, std::vector<TaskHandle>& outDependants) noexcept {
	TRIVIAL_ASSERT(handle.isValid());

	TaskSlot* slot = slotAt(handle.index);

	TRIVIAL_ASSERT(slot != nullptr);

	std::lock_guard<TaskSlotMutex> lock(slot->mutex());

	TRIVIAL_ASSERT(slot->isOccupiedBy(handle));

	TaskState& state = slot->state();

	TRIVIAL_ASSERT(state.status() == TaskStatus::Running);

	state.setStatus(TaskStatus::Completed);

	outDependants.clear();
	outDependants.insert(outDependants.end(), state.dependants.begin(), state.dependants.end());

	if (state.waitGroup != nullptr) {
		TaskWaitGroup::onMemberComplete(state.waitGroup);
		state.waitGroup = nullptr;
	}
}

bool TaskGraph::removePrerequisiteAndMarkReadyIfUnblocked(TaskHandle dependant,
                                                          TaskHandle prerequisite,
                                                          TaskReadyInfo& outReadyInfo) noexcept {
	TRIVIAL_ASSERT(dependant.isValid());
	TRIVIAL_ASSERT(prerequisite.isValid());

	TaskSlot* dependantSlot = slotAt(dependant.index);

	TRIVIAL_ASSERT(dependantSlot != nullptr);

	std::lock_guard<TaskSlotMutex> lock(dependantSlot->mutex());

	TRIVIAL_ASSERT(dependantSlot->isOccupiedBy(dependant));

	TaskState& dependantState = dependantSlot->state();

	TRIVIAL_ASSERT(dependantState.status() == TaskStatus::Waiting);

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

	dependantState.setStatus(TaskStatus::Ready);
	outReadyInfo.priority = dependantState.priority();
	outReadyInfo.affinity = dependantState.affinity();

	return true;
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

		if (state.status() != TaskStatus::Completed && state.status() != TaskStatus::Cancelled) {
			return TaskReleaseResult::TaskNotComplete;
		}

		slot->destroy();
	}

	releaseTaskIndex(handle.index);

	return TaskReleaseResult::Success;
}

void TaskGraph::detachWaiterIfUnclaimed(TaskHandle handle, const TaskWaitGroup& waitGroup) noexcept {
	TRIVIAL_ASSERT(handle.isValid());

	TaskSlot* slot = slotAt(handle.index);

	TRIVIAL_ASSERT(slot != nullptr);

	std::lock_guard<TaskSlotMutex> lock(slot->mutex());

	if (!slot->isOccupiedBy(handle)) {
		return;
	}

	TaskState& state = slot->state();

	if (state.waitGroup == &waitGroup) {
		state.waitGroup = nullptr;
	}
}

bool TaskGraph::tryGetStatus(TaskHandle handle, TaskStatus& outStatus) const noexcept {
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

	outStatus = slot->state().status();

	return true;
}

bool TaskGraph::tryGetWalkInfo(TaskHandle handle,
                               TaskWalkInfo& outInfo,
                               std::vector<TaskHandle>& outPrerequisites) const noexcept {
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

	const TaskState& state = slot->state();

	outInfo.status = state.status();
	outInfo.affinity = state.affinity();

	outPrerequisites.clear();

	if (outInfo.status == TaskStatus::Waiting) {
		outPrerequisites.insert(outPrerequisites.end(), state.prerequisites.begin(), state.prerequisites.end());
	}

	return true;
}

void* TaskGraph::getResultPointer(TaskHandle handle) noexcept {
	TRIVIAL_ASSERT(handle.isValid());

	TaskSlot* slot = slotAt(handle.index);

	TRIVIAL_ASSERT(slot != nullptr);

	std::lock_guard<TaskSlotMutex> lock(slot->mutex());

	TRIVIAL_ASSERT(slot->isOccupiedBy(handle));

	TaskState& state = slot->state();

	TRIVIAL_ASSERT(state.status() == TaskStatus::Completed);

	return state.payload.getResultPointer();
}

TaskGraph::TaskPage* TaskGraph::pageAt(std::uint32_t pageIndex) noexcept {
	if (pageIndex >= kMaxPageCount) {
		return nullptr;
	}

	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
	return m_pages[pageIndex].load(std::memory_order_acquire);
}

const TaskGraph::TaskPage* TaskGraph::pageAt(std::uint32_t pageIndex) const noexcept {
	if (pageIndex >= kMaxPageCount) {
		return nullptr;
	}

	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
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

	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
	page = m_pages[pageIndex].load(std::memory_order_relaxed);

	if (page != nullptr) {
		return page;
	}

	TaskPage* newPage = new (std::nothrow) TaskPage;

	if (newPage == nullptr) {
		TRIVIAL_LOG_ERROR("TaskGraph failed to allocate task page");
		return nullptr;
	}

	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
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
	return &page[slotIndexFor(taskIndex)]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
}

const TaskSlot* TaskGraph::slotInPage(const TaskPage& page, std::uint32_t taskIndex) noexcept {
	return &page[slotIndexFor(taskIndex)]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
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

TaskPrerequisiteResult TaskGraph::addPrerequisiteLocked(TaskHandle dependantHandle,
                                                        TaskSlot& dependantSlot,
                                                        TaskHandle prerequisiteHandle) noexcept {
	TRIVIAL_ASSERT(dependantHandle.isValid());
	TRIVIAL_ASSERT(prerequisiteHandle.isValid());

	if (dependantHandle == prerequisiteHandle) {
		return TaskPrerequisiteResult::SelfDependency;
	}

#if TRIVIAL_CONFIG_DEBUG
	std::lock_guard<TaskGraphMutex> topologyLock(m_debugTopologyMutex);

	if (wouldCreateCycle(dependantHandle, dependantSlot, prerequisiteHandle)) {
		TRIVIAL_LOG_ERROR("Task graph tried to create a dependency loop/cycle");

		return TaskPrerequisiteResult::InvalidState;
	}
#endif // TRIVIAL_CONFIG_DEBUG

	TaskSlot* prerequisiteSlot = slotAt(prerequisiteHandle.index);

	if (prerequisiteSlot == nullptr) {
		return TaskPrerequisiteResult::InvalidHandle;
	}

	std::lock_guard<TaskSlotMutex> prerequisiteLock(prerequisiteSlot->mutex());

	if (!prerequisiteSlot->isOccupiedBy(prerequisiteHandle)) {
		return TaskPrerequisiteResult::InvalidHandle;
	}

	TRIVIAL_ASSERT(dependantSlot.isOccupiedBy(dependantHandle));

	TaskState& dependantState = dependantSlot.state();
	TaskState& prerequisiteState = prerequisiteSlot->state();

	TRIVIAL_ASSERT(dependantState.status() == TaskStatus::Created);

	if (prerequisiteState.status() == TaskStatus::Cancelled) {
		return TaskPrerequisiteResult::InvalidState;
	}

	for (const TaskHandle& existingPrerequisite : dependantState.prerequisites) {
		if (existingPrerequisite == prerequisiteHandle) {
			return TaskPrerequisiteResult::DuplicateDependency;
		}
	}

	if (prerequisiteState.status() == TaskStatus::Completed) {
		return TaskPrerequisiteResult::Success;
	}

	dependantState.prerequisites.push_back(prerequisiteHandle);
	prerequisiteState.dependants.push_back(dependantHandle);

	return TaskPrerequisiteResult::Success;
}

#if TRIVIAL_CONFIG_DEBUG

bool TaskGraph::wouldCreateCycle(TaskHandle taskHandle,
                                 const TaskSlot& taskSlot,
                                 TaskHandle prerequisiteHandle) const noexcept {
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

		if (kCurrentHandle == taskHandle) {
			for (const TaskHandle& dependantHandle : taskSlot.state().dependants) {
				dependantStack.push_back(dependantHandle);
			}
			continue;
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

#endif // TRIVIAL_CONFIG_DEBUG

} // namespace trivial::task

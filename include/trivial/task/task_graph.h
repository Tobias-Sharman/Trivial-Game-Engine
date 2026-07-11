#ifndef TRIVIAL_TASK_TASK_GRAPH_H
#define TRIVIAL_TASK_TASK_GRAPH_H

#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

#include <trivial/core/config.h>
#include <trivial/task/task_handle.h>
#include <trivial/task/task_launch_options.h>
#include <trivial/task/task_mutex.h>
#include <trivial/task/task_payload.h>
#include <trivial/task/task_slot.h>
#include <trivial/task/task_status.h>

namespace trivial::task {

enum class TaskCreateResult : std::uint8_t {
	Success,
	CapacityExhausted,
	AllocationFailure
};

enum class TaskPrerequisiteResult : std::uint8_t {
	Success,
	InvalidHandle,
	InvalidState,
	DuplicateDependency,
	SelfDependency
};

enum class TaskDispatchResult : std::uint8_t {
	Success,
	InvalidHandle,
	AlreadyDispatched
};

enum class TaskReadiness : std::uint8_t {
	Waiting,
	Ready
};

enum class TaskClaimResult : std::uint8_t {
	Success,
	InvalidHandle,
	NotReady,
};

enum class TaskReleaseResult : std::uint8_t {
	Success,
	InvalidHandle,
	TaskNotComplete
};

struct TaskCreateOutcome {
	TaskCreateResult result = TaskCreateResult::CapacityExhausted;
	TaskHandle handle{};
};

struct TaskDispatchOutcome {
	TaskDispatchResult result = TaskDispatchResult::InvalidHandle;
	TaskReadiness readiness = TaskReadiness::Waiting;
	TaskPriority priority{};
};

class TaskGraph {
public:
	TaskGraph() noexcept = default;

	~TaskGraph() noexcept;

	TaskGraph(const TaskGraph&) = delete;
	TaskGraph& operator=(const TaskGraph&) = delete;

	TaskGraph(TaskGraph&&) = delete;
	TaskGraph& operator=(TaskGraph&&) = delete;

	[[nodiscard]] TaskCreateOutcome create(TaskPayload payload, const TaskLaunchOptions& options) noexcept;

	[[nodiscard]] TaskPrerequisiteResult addPrerequisite(TaskHandle taskHandle, TaskHandle prerequisiteHandle) noexcept;

	[[nodiscard]] TaskDispatchOutcome dispatch(TaskHandle handle) noexcept;

	[[nodiscard]] TaskClaimResult tryClaim(TaskHandle handle) noexcept;

	void executeClaimed(TaskHandle handle) noexcept;

	void beginCompletion(TaskHandle handle, std::vector<TaskHandle>& outDependants) noexcept;

	// Unideal but means that only one lock is needed. True for ready to be enqueue, false if not
	[[nodiscard]] bool removePrerequisiteAndMarkReadyIfUnblocked(TaskHandle dependantHandle,
	                                                             TaskHandle prerequisiteHandle,
	                                                             TaskPriority& readyPriority) noexcept;

	void finishCompletion(TaskHandle handle) noexcept;

	[[nodiscard]] TaskReleaseResult release(TaskHandle handle) noexcept;

	[[nodiscard]] bool tryGetStatus(TaskHandle handle, TaskStatus& status) const noexcept;

private:
	static constexpr std::uint32_t kTaskSlotsPerPage = 256;
	static constexpr std::uint32_t kMaxTaskCount = 65'536;

	static constexpr std::uint32_t kMaxPageCount
	    = (kMaxTaskCount / kTaskSlotsPerPage) + static_cast<std::uint32_t>(kMaxTaskCount % kTaskSlotsPerPage != 0);

	using TaskPage = std::array<TaskSlot, kTaskSlotsPerPage>;

	[[nodiscard]] static constexpr std::uint32_t pageIndexFor(std::uint32_t taskIndex) noexcept {
		return taskIndex / kTaskSlotsPerPage;
	}

	[[nodiscard]] static constexpr std::uint32_t slotIndexFor(std::uint32_t taskIndex) noexcept {
		return taskIndex % kTaskSlotsPerPage;
	}

	[[nodiscard]] TaskPage* pageAt(std::uint32_t pageIndex) noexcept;
	[[nodiscard]] const TaskPage* pageAt(std::uint32_t pageIndex) const noexcept;

	[[nodiscard]] TaskPage* ensurePage(std::uint32_t pageIndex) noexcept;

	[[nodiscard]] TaskSlot* slotAt(std::uint32_t taskIndex) noexcept;
	[[nodiscard]] const TaskSlot* slotAt(std::uint32_t task) const noexcept;

	// NOTE: Slot in page does not check fot validity of page and task index
	[[nodiscard]] static TaskSlot* slotInPage(TaskPage& page, std::uint32_t taskIndex) noexcept;
	[[nodiscard]] static const TaskSlot* slotInPage(const TaskPage& page, std::uint32_t taskIndex) noexcept;

	[[nodiscard]] bool allocateTaskIndex(std::uint32_t& taskIndex) noexcept;

	void releaseTaskIndex(std::uint32_t taskIndex) noexcept;

#if TRIVIAL_CONFIG_DEBUG
	[[nodiscard]] bool wouldCreateCycle(TaskHandle taskHandle, TaskHandle prerequisiteHandle) const noexcept;
#endif // TRIVIAL_CONFIG_DEBUG

	std::array<std::atomic<TaskPage*>, kMaxPageCount> m_pages{};

	TaskGraphMutex m_pageCreationMutex;
	TaskGraphMutex m_allocationMutex;

#if TRIVIAL_CONFIG_DEBUG
	TaskGraphMutex m_debugTopologyMutex;
#endif // TRIVIAL_CONFIG_DEBUG

	std::vector<std::uint32_t> m_freeTaskIndices; // TODO: replace when having custom allocator
	std::uint32_t m_nextUnusedTaskIndex = 0;

	static_assert(kTaskSlotsPerPage > 0);
	static_assert(kMaxTaskCount > 0);
	static_assert(kMaxPageCount > 0);
};

} // namespace trivial::task

#endif // TRIVIAL_TASK_TASK_GRAPH_H

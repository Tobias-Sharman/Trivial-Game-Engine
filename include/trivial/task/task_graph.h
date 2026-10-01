#ifndef TRIVIAL_TASK_TASK_GRAPH_H
#define TRIVIAL_TASK_TASK_GRAPH_H

#include <array>
#include <atomic>
#include <cstdint>
#include <span>
#include <vector>

#include <trivial/core/compiler.h>
#include <trivial/core/config.h>
#include <trivial/core/sync/latch.h>
#include <trivial/core/sync/mutex.h>
#include <trivial/task/task_handle.h>
#include <trivial/task/task_launch_options.h>
#include <trivial/task/task_payload.h>
#include <trivial/task/task_slot.h>
#include <trivial/task/task_status.h>
#include <trivial/task/task_system_config.h>

namespace trivial::task {

enum class TaskCreateResult : std::uint8_t {
	Success,
	CapacityExhausted,
	AllocationFailure,
};

enum class TaskPrerequisiteResult : std::uint8_t {
	Success,
	InvalidHandle,
	InvalidState,
	DuplicateDependency,
	SelfDependency,
};

enum class TaskDispatchResult : std::uint8_t {
	Success,
	InvalidHandle,
	AlreadyDispatched,
};

enum class TaskReadiness : std::uint8_t {
	Waiting,
	Ready,
};

enum class TaskClaimResult : std::uint8_t {
	Success,
	InvalidHandle,
	NotReady,
};

enum class TaskAttachWaiterResult : std::uint8_t {
	AlreadyComplete,
	Attached,
	InvalidHandle,
};

enum class TaskReleaseResult : std::uint8_t {
	Success,
	InvalidHandle,
	TaskNotComplete,
};

struct TaskCreateDispatchOutcome {
	TaskCreateResult createResult = TaskCreateResult::CapacityExhausted;
	TaskHandle handle{};
	TaskReadiness readiness = TaskReadiness::Waiting;
	TaskPriority priority{};
};

struct TaskWalkInfo {
	TaskStatus status = TaskStatus::Created;
	TaskAffinity affinity = TaskAffinity::AnyWorker;
};

struct TaskReadyInfo {
	TaskPriority priority = TaskPriority::Normal;
	TaskAffinity affinity = TaskAffinity::AnyWorker;
};

class TaskGraph {
public:
	TaskGraph() noexcept = default;

	~TaskGraph() noexcept;

	TaskGraph(const TaskGraph&) = delete;
	TaskGraph& operator=(const TaskGraph&) = delete;

	TaskGraph(TaskGraph&&) = delete;
	TaskGraph& operator=(TaskGraph&&) = delete;

	[[nodiscard]] TRIVIAL_HOT TaskCreateDispatchOutcome createDispatched(TaskPayload payload,
	                                                                     std::span<const TaskHandle> prerequisites,
	                                                                     const TaskLaunchOptions& options) noexcept;

	[[nodiscard]] TRIVIAL_HOT TaskClaimResult tryClaim(TaskHandle handle) noexcept;

	[[nodiscard]] TaskAttachWaiterResult tryAttachWaiter(TaskHandle handle, sync::Latch& latch) noexcept;

	TRIVIAL_HOT void executeClaimed(TaskHandle handle) noexcept;

	TRIVIAL_HOT void completeAndCollectDependants(TaskHandle handle, std::vector<TaskHandle>& outDependants) noexcept;

	// Unideal but means that only one lock is needed. True for ready to be
	// enqueue, false if not
	[[nodiscard]] TRIVIAL_HOT bool removePrerequisiteAndMarkReadyIfUnblocked(TaskHandle dependantHandle,
	                                                                         TaskHandle prerequisiteHandle,
	                                                                         TaskReadyInfo& outReadyInfo) noexcept;

	[[nodiscard]] TaskReleaseResult release(TaskHandle handle) noexcept;

	[[nodiscard]] bool tryGetStatus(TaskHandle handle, TaskStatus& outStatus) const noexcept;

	[[nodiscard]] bool tryGetWalkInfo(TaskHandle handle,
	                                  TaskWalkInfo& outInfo,
	                                  std::vector<TaskHandle>& outPrerequisites) const noexcept;

	[[nodiscard]] void* getResultPointer(TaskHandle handle) noexcept TRIVIAL_LIFETIMEBOUND;

private:
	using TaskPage = std::array<TaskSlot, TRIVIAL_TASK_GRAPH_SLOTS_PER_PAGE>;

	[[nodiscard]] static constexpr std::uint32_t pageIndexFor(std::uint32_t taskIndex) noexcept {
		return taskIndex / TRIVIAL_TASK_GRAPH_SLOTS_PER_PAGE;
	}

	[[nodiscard]] static constexpr std::uint32_t slotIndexFor(std::uint32_t taskIndex) noexcept {
		return taskIndex % TRIVIAL_TASK_GRAPH_SLOTS_PER_PAGE;
	}

	[[nodiscard]] TaskPage* pageAt(std::uint32_t pageIndex) noexcept TRIVIAL_LIFETIMEBOUND;
	[[nodiscard]] const TaskPage* pageAt(std::uint32_t pageIndex) const noexcept TRIVIAL_LIFETIMEBOUND;

	[[nodiscard]] TaskPage* ensurePage(std::uint32_t pageIndex) noexcept TRIVIAL_LIFETIMEBOUND;

	[[nodiscard]] TaskSlot* slotAt(std::uint32_t taskIndex) noexcept TRIVIAL_LIFETIMEBOUND;
	[[nodiscard]] const TaskSlot* slotAt(std::uint32_t task) const noexcept TRIVIAL_LIFETIMEBOUND;

	// NOTE: Slot in page does not check for validity of page and task index
	[[nodiscard]] static TaskSlot* slotInPage(TaskPage& page, std::uint32_t taskIndex) noexcept;
	[[nodiscard]] static const TaskSlot* slotInPage(const TaskPage& page, std::uint32_t taskIndex) noexcept;

	[[nodiscard]] bool allocateTaskIndex(std::uint32_t& taskIndex) noexcept;

	void releaseTaskIndex(std::uint32_t taskIndex) noexcept;

	[[nodiscard]] TaskPrerequisiteResult addPrerequisiteLocked(TaskHandle dependantHandle,
	                                                           TaskSlot& dependantSlot,
	                                                           TaskHandle prerequisiteHandle) noexcept;
#if TRIVIAL_CONFIG_DEBUG
	[[nodiscard]] bool wouldCreateCycle(TaskHandle taskHandle,
	                                    const TaskSlot& taskSlot,
	                                    TaskHandle prerequisiteHandle) const noexcept;
#endif // TRIVIAL_CONFIG_DEBUG

	std::array<std::atomic<TaskPage*>, TRIVIAL_TASK_GRAPH_MAX_PAGE_COUNT> m_pages{};

	sync::Mutex m_pageCreationMutex;
	sync::Mutex m_allocationMutex;

#if TRIVIAL_CONFIG_DEBUG
	sync::Mutex m_debugTopologyMutex;
#endif // TRIVIAL_CONFIG_DEBUG

	std::vector<std::uint32_t> m_freeTaskIndices; // TODO: replace when having custom allocator
	std::uint32_t m_nextUnusedTaskIndex = 0;
};

} // namespace trivial::task

#endif // TRIVIAL_TASK_TASK_GRAPH_H

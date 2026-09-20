#ifndef TRIVIAL_TASK_TASK_SYSTEM_H
#define TRIVIAL_TASK_TASK_SYSTEM_H

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <limits>
#include <span>
#include <vector>

#include <trivial/core/platform.h>
#include <trivial/core/sync/mutex.h>
#include <trivial/core/sync/semaphore.h>
#include <trivial/task/task_graph.h>
#include <trivial/task/task_handle.h>
#include <trivial/task/task_launch_options.h>
#include <trivial/task/task_payload.h>
#include <trivial/task/task_priority_queue.h>
#include <trivial/task/task_system_config.h>
#include <trivial/task/worker.h>

#if TRIVIAL_PLATFORM_POSIX
#include <trivial/core/thread/thread_stack_allocator.h>
#endif // TRIVIAL_PLATFORM_POSIX

namespace trivial::task {

// TODO: Support for thread affinities beyond main

class TaskSystem {
public:
	explicit TaskSystem(const TaskSystemConfig& config);

	~TaskSystem() noexcept;

	TaskSystem(const TaskSystem&) = delete;
	TaskSystem& operator=(const TaskSystem&) = delete;

	TaskSystem(TaskSystem&&) = delete;
	TaskSystem& operator=(TaskSystem&&) = delete;

	[[nodiscard]] TaskHandle launch(TaskPayload payload, const TaskLaunchOptions& options = {}) noexcept;

	[[nodiscard]] TaskHandle launch(TaskPayload payload,
	                                TaskHandle prerequisite,
	                                const TaskLaunchOptions& options = {}) noexcept;

	// TODO: Add more non general paths

	[[nodiscard]] TaskHandle launch(TaskPayload payload,
	                                std::span<const TaskHandle> prerequisites,
	                                const TaskLaunchOptions& options = {}) noexcept;

	void wait(TaskHandle task) noexcept;
	void wait(std::span<const TaskHandle> tasks) noexcept;

	[[nodiscard]] void* getResultPointer(TaskHandle handle) noexcept;

	[[nodiscard]] bool isComplete(TaskHandle task) const noexcept;

	[[nodiscard]] TaskReleaseResult release(TaskHandle task) noexcept;

	void runMainThreadReadyTasks() noexcept;

private:
	static constexpr std::size_t kInvalidWorkerIndex = std::numeric_limits<std::size_t>::max();

	[[nodiscard]] std::size_t tryGetCurrentWorkerIndex() const noexcept;

	static void workerThreadEntry(void* arg) noexcept;

	void runWorkerLoop(std::size_t workerIndex);

	[[nodiscard]] bool parkWorker(std::size_t workerIndex) noexcept;
	void wakeWorker(std::size_t workerIndex) noexcept;

	void wakeOneIfUnderTarget() noexcept;

	void removeParkedIndex(std::size_t workerIndex) noexcept;

	[[nodiscard]] bool tryStealTask(std::size_t workerIndex, TaskHandle& handle) noexcept;

	void enqueueReadyTask(TaskHandle handle, TaskAffinity affinity, TaskPriority priority) noexcept;

	void completeTask(TaskHandle handle) noexcept;

	[[nodiscard]] bool tryPopAndRunOneAnyWorkerTask() noexcept;

	void runAndCompleteClaimedTask(TaskHandle handle) noexcept;

	[[nodiscard]] bool tryHelpComplete(TaskHandle target, TaskAffinity callerAffinity, std::uint32_t maxDepth) noexcept;

	TaskGraph m_graph;

	std::array<TaskPriorityQueue, static_cast<std::size_t>(TaskAffinity::Count)> m_affinityQueues;

#if TRIVIAL_PLATFORM_POSIX
	thread::ThreadStackAllocator m_workerStackAllocator;
#endif // TRIVIAL_PLATFORM_POSIX

	// Can't do vector since no move construction
	std::deque<Worker> m_workers; // TODO: Custom container with custom allocator since deque makes no semantic sense

	std::atomic<std::size_t> m_nextWorkerStartIndex{0};

	std::size_t m_targetActiveWorkerCount = 0;
	sync::Semaphore m_activeSlots;

	sync::Mutex m_parkedIndicesMutex;
	std::vector<std::size_t> m_parkedWorkerIndices; // TODO: custom allocator/structure

	std::uint32_t m_waitHelpMaxDepth = 0;
};

} // namespace trivial::task

#endif // TRIVIAL_TASK_TASK_SYSTEM_H

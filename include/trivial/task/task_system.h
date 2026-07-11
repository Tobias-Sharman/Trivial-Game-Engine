#ifndef TRIVIAL_TASK_TASK_SYSTEM_H
#define TRIVIAL_TASK_TASK_SYSTEM_H

#include <array>
#include <cstddef>
#include <deque>
#include <span>
#include <vector>

#include <trivial/task/task_graph.h>
#include <trivial/task/task_handle.h>
#include <trivial/task/task_launch_options.h>
#include <trivial/task/task_mutex.h>
#include <trivial/task/task_payload.h>

namespace trivial::task {

class TaskSystem {
public:
	TaskSystem() noexcept = default;
	// TODO: Proper creation and destruction when adding workers and queues
	~TaskSystem() noexcept = default;

	TaskSystem(const TaskSystem&) = delete;
	TaskSystem& operator=(const TaskSystem&) = delete;

	TaskSystem(TaskSystem&&) = delete;
	TaskSystem& operator=(const TaskSystem&&) = delete;

	[[nodiscard]] TaskHandle launch(TaskPayload payload, const TaskLaunchOptions& options = {}) noexcept;

	[[nodiscard]] TaskHandle launch(TaskPayload payload,
	                                TaskHandle prerequisite,
	                                const TaskLaunchOptions& options = {}) noexcept;

	// TODO: Add more non general paths

	[[nodiscard]] TaskHandle launch(TaskPayload payload,
	                                std::span<const TaskHandle> prerequisites,
	                                const TaskLaunchOptions& options = {}) noexcept;

	void wait(TaskHandle task) noexcept;
	void wait(std::span<const TaskHandle> tasks) noexcept; // TODO: add more general tasks

	[[nodiscard]] bool isComplete(TaskHandle task) const noexcept;

	[[nodiscard]] TaskReleaseResult release(TaskHandle task) noexcept;

private:
	void enqueueReadyTask(TaskHandle handle, TaskPriority priority) noexcept;

	[[nodiscard]] bool tryPopReadyTask(TaskHandle& handle) noexcept;

	[[nodiscard]] bool executeOneReadyTask() noexcept;

	void completeTask(TaskHandle handle) noexcept;

	TaskGraph m_graph;
	TaskGraphMutex m_readyMutex;

	using ReadyQueue = std::deque<TaskHandle>; // TODO: custom deque
	std::array<ReadyQueue, static_cast<std::size_t>(TaskPriority::Count)> m_readyQueues;
	// TODO: custom allocator
	std::vector<TaskHandle> m_completionDependants;
};

} // namespace trivial::task

#endif // TRIVIAL_TASK_TASK_SYSTEM_H

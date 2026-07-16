#include <trivial/task/task_system.h>

#include <cstddef>
#include <mutex>
#include <utility>
#include <vector>

#include <trivial/core/assert.h>

namespace trivial::task {

TaskHandle TaskSystem::launch(TaskPayload payload, const TaskLaunchOptions& options) noexcept {
	const TaskCreateOutcome kCreateOutcome = m_graph.create(std::move(payload), options);

	if (kCreateOutcome.result != TaskCreateResult::Success) {
		return {};
	}

	const TaskDispatchOutcome kDispatchOutcome = m_graph.dispatch(kCreateOutcome.handle);

	TRIVIAL_ASSERT(kDispatchOutcome.result == TaskDispatchResult::Success);

	if (kDispatchOutcome.readiness == TaskReadiness::Ready) {
		enqueueReadyTask(kCreateOutcome.handle, kDispatchOutcome.priority);
	}

	return kCreateOutcome.handle;
}

TaskHandle TaskSystem::launch(TaskPayload payload, TaskHandle prerequisite, const TaskLaunchOptions& options) noexcept {
	const TaskCreateOutcome kCreateOutcome = m_graph.create(std::move(payload), options);

	if (kCreateOutcome.result != TaskCreateResult::Success) {
		return {};
	}

	const TaskPrerequisiteResult kPrerequisiteResult = m_graph.addPrerequisite(kCreateOutcome.handle, prerequisite);

	TRIVIAL_ASSERT(kPrerequisiteResult == TaskPrerequisiteResult::Success);

	const TaskDispatchOutcome kDispatchOutcome = m_graph.dispatch(kCreateOutcome.handle);

	TRIVIAL_ASSERT(kDispatchOutcome.result == TaskDispatchResult::Success);

	if (kDispatchOutcome.readiness == TaskReadiness::Ready) {
		enqueueReadyTask(kCreateOutcome.handle, kDispatchOutcome.priority);
	}

	return kCreateOutcome.handle;
}

TaskHandle TaskSystem::launch(TaskPayload payload,
                              std::span<const TaskHandle> prerequisites,
                              const TaskLaunchOptions& options) noexcept {
	if (prerequisites.empty()) {
		return launch(std::move(payload), options);
	}

	if (prerequisites.size() == 1) {
		return launch(std::move(payload), prerequisites[0], options);
	}

	const TaskCreateOutcome kCreateOutcome = m_graph.create(std::move(payload), options);

	if (kCreateOutcome.result != TaskCreateResult::Success) {
		return {};
	}

	for (TaskHandle prerequisite : prerequisites) {
		const TaskPrerequisiteResult kPrerequisiteResult = m_graph.addPrerequisite(kCreateOutcome.handle, prerequisite);

		TRIVIAL_ASSERT(kPrerequisiteResult == TaskPrerequisiteResult::Success);
	}

	const TaskDispatchOutcome kDispatchOutcome = m_graph.dispatch(kCreateOutcome.handle);

	TRIVIAL_ASSERT(kDispatchOutcome.result == TaskDispatchResult::Success);

	if (kDispatchOutcome.readiness == TaskReadiness::Ready) {
		enqueueReadyTask(kCreateOutcome.handle, kDispatchOutcome.priority);
	}

	return kCreateOutcome.handle;
}

void TaskSystem::wait(TaskHandle task) noexcept {
	TRIVIAL_ASSERT(task.isValid());

	while (!isComplete(task)) {
		const bool kExecutedTask = executeOneReadyTask();

		TRIVIAL_ASSERT(kExecutedTask);
	}
}

void TaskSystem::wait(std::span<const TaskHandle> tasks) noexcept {
	while (true) {
		bool allComplete = true;

		for (TaskHandle task : tasks) {
			TRIVIAL_ASSERT(task.isValid());

			if (!isComplete(task)) {
				allComplete = false;
				break;
			}
		}

		if (allComplete) {
			return;
		}

		const bool kExecutedTask = executeOneReadyTask();

		TRIVIAL_ASSERT(kExecutedTask);
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

void TaskSystem::enqueueReadyTask(TaskHandle handle, TaskPriority priority) noexcept {
	TRIVIAL_ASSERT(handle.isValid());
	TRIVIAL_ASSERT(priority < TaskPriority::Count);

	std::lock_guard<TaskGraphMutex> lock(m_readyMutex);

	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
	m_readyQueues[static_cast<std::size_t>(priority)].push_back(handle);
}

bool TaskSystem::tryPopReadyTask(TaskHandle& handle) noexcept {
	std::lock_guard<TaskGraphMutex> lock(m_readyMutex);

	for (std::size_t priorityIndex = static_cast<std::size_t>(TaskPriority::Count); priorityIndex > 0;
	     --priorityIndex) {
		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
		ReadyQueue& queue = m_readyQueues[priorityIndex - 1];

		if (queue.empty()) {
			continue;
		}

		handle = queue.front();
		queue.pop_front();

		return true;
	}

	return false;
}

bool TaskSystem::executeOneReadyTask() noexcept {
	TaskHandle handle{};

	if (!tryPopReadyTask(handle)) {
		return false;
	}

	const TaskClaimResult kClaimResult = m_graph.tryClaim(handle);

	if (kClaimResult != TaskClaimResult::Success) {
		return false;
	}

	m_graph.executeClaimed(handle);

	completeTask(handle);

	return true;
}

void TaskSystem::completeTask(TaskHandle handle) noexcept {
	std::vector<TaskHandle> completionDependants; // TODO: Make thread local vectors repeat allocations are poor
	m_graph.beginCompletion(handle, completionDependants);

	for (TaskHandle dependant : completionDependants) {
		TaskPriority readyPriority = TaskPriority::Normal;

		if (m_graph.removePrerequisiteAndMarkReadyIfUnblocked(dependant, handle, readyPriority)) {
			enqueueReadyTask(dependant, readyPriority);
		}
	}

	m_graph.finishCompletion(handle);
}

} // namespace trivial::task

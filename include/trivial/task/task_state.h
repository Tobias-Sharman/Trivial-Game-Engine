#ifndef TRIVIAL_SRC_TASK_TASK_STATE_H
#define TRIVIAL_SRC_TASK_TASK_STATE_H

#include <utility>
#include <vector>

#include <trivial/task/task_handle.h>
#include <trivial/task/task_launch_options.h>
#include <trivial/task/task_payload.h>
#include <trivial/task/task_status.h>

namespace trivial::task {

// TODO: Measure size, padding, and alignment on different platforms for cache optimisation, can very likely be improved
//       since now is more of a basic semantic sense (decenet for cache not optimised for size which is significant
//       here)
struct TaskState {
	TaskState(TaskPayload payload, const TaskLaunchOptions& options) noexcept
	    : payload(std::move(payload))
	    , basePriority(options.priority)
	    , effectivePriority(options.priority)
	    , affinity(options.affinity)
	    , lifetime(options.lifetime)
	    , scope(options.scope) {}

	~TaskState() = default;

	TaskState(const TaskState&) = delete;
	TaskState& operator=(const TaskState&) = delete;

	TaskState(TaskState&&) = delete;
	TaskState& operator=(TaskState&&) = delete;

	TaskStatus status = TaskStatus::Created;

	TaskPayload payload;
	std::vector<TaskHandle> dependants;
	std::vector<TaskHandle> prerequisites;

	TaskPriority basePriority = TaskPriority::Normal;
	TaskPriority effectivePriority = TaskPriority::Normal;
	TaskAffinity affinity = TaskAffinity::AnyWorker;
	TaskLifetime lifetime = TaskLifetime::AutoRelease;
	TaskScopeHandle scope = {};
};

} // namespace trivial::task

#endif // TRIVIAL_SRC_TASK_TASK_STATE_H

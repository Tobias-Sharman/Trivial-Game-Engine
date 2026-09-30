#ifndef TRIVIAL_TASK_TASK_LAUNCH_OPTIONS_H
#define TRIVIAL_TASK_TASK_LAUNCH_OPTIONS_H

#include <cstdint>
#include <limits>

#define TRIVIAL_TASK_SCOPE_HANDLE_INVALID_INDEX (std::numeric_limits<std::uint32_t>::max())

namespace trivial::task {

enum class TaskAffinity : std::uint8_t {
	AnyWorker,
	MainThread,
	// RenderThread,

	Count,
};

enum class TaskPriority : std::uint8_t {
	Background,
	Normal,
	High,
	Critical,

	Count,
};

enum class TaskLifetime : std::uint8_t {
	AutoRelease,
	Manual,
};

struct TaskScopeHandle {
	std::uint32_t index = TRIVIAL_TASK_SCOPE_HANDLE_INVALID_INDEX;
	std::uint32_t generation = 0;

	[[nodiscard]] bool isValid() const noexcept { return index != TRIVIAL_TASK_SCOPE_HANDLE_INVALID_INDEX; }
};

struct TaskLaunchOptions {
	TaskAffinity affinity = TaskAffinity::AnyWorker;
	TaskPriority priority = TaskPriority::Normal;
	TaskScopeHandle scope = {};
	TaskLifetime lifetime = TaskLifetime::AutoRelease;
};

// TODO: Reduce scope handle size, should not need to have such a large amount of indices or generation

} // namespace trivial::task

#undef TRIVIAL_TASK_SCOPE_HANDLE_INVALID_INDEX

#endif // TRIVIAL_TASK_TASK_LAUNCH_OPTIONS_H

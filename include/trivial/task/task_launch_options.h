#ifndef TRIVIAL_TASK_TASK_LAUNCH_OPTIONS_H
#define TRIVIAL_TASK_TASK_LAUNCH_OPTIONS_H

#include <cstdint>
#include <limits>

namespace trivial::task {

enum class TaskAffinity : std::uint8_t {
	AnyWorker,
	MainThread,
	// RenderThread,

	Count
};

enum class TaskPriority : std::uint8_t {
	Background,
	Normal,
	High,
	Critical,

	Count
};

enum class TaskLifetime : std::uint8_t {
	AutoRelease,
	Manual
};

struct TaskScopeHandle {
	static constexpr std::uint32_t kInvalidIndex = std::numeric_limits<std::uint32_t>::max();

	std::uint32_t index = kInvalidIndex;
	std::uint32_t generation = 0;

	[[nodiscard]] bool isValid() const noexcept { return index != kInvalidIndex; }
};

struct TaskLaunchOptions {
	TaskAffinity affinity = TaskAffinity::AnyWorker;
	TaskPriority priority = TaskPriority::Normal;
	TaskScopeHandle scope = {};
	TaskLifetime lifetime = TaskLifetime::AutoRelease;
};

// TODO: Reduce scope handle size, should not need to have such a large amount of indices or generation

} // namespace trivial::task

#endif // TRIVIAL_TASK_TASK_LAUNCH_OPTIONS_H

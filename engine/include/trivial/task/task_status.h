#ifndef TRIVIAL_TASK_TASK_STATUS_H
#define TRIVIAL_TASK_TASK_STATUS_H

#include <cstdint>

namespace trivial::task {

enum class TaskStatus : std::uint8_t {
	Created,
	Waiting,
	Ready,
	Running,
	Completed,
	Cancelled,
};

} // namespace trivial::task

#endif // TRIVIAL_TASK_TASK_STATUS_H

#ifndef TRIVIAL_TASK_TASK_HANDLE_H
#define TRIVIAL_TASK_TASK_HANDLE_H

#include <cstdint>
#include <limits>

#define TRIVIAL_TASK_HANDLE_INVALID_INDEX (std::numeric_limits<std::uint32_t>::max())

namespace trivial::task {

struct TaskHandle {
	std::uint32_t index = TRIVIAL_TASK_HANDLE_INVALID_INDEX;
	std::uint32_t generation = 0;

	[[nodiscard]] constexpr bool operator==(const TaskHandle& other) const noexcept {
		return index == other.index && generation == other.generation;
	}
	[[nodiscard]] constexpr bool isValid() const noexcept { return index != TRIVIAL_TASK_HANDLE_INVALID_INDEX; }
};

} // namespace trivial::task

#undef TRIVIAL_TASK_HANDLE_INVALID_INDEX

#endif // TRIVIAL_TASK_TASK_HANDLE_H

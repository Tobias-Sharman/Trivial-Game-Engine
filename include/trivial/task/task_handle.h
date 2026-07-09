#ifndef TRIVIAL_TASK_TASK_HANDLE_H
#define TRIVIAL_TASK_TASK_HANDLE_H

#include <cstdint>
#include <limits>

namespace trivial::task {

struct TaskHandle {
	static constexpr std::uint32_t kInvalidIndex = std::numeric_limits<std::uint32_t>::max();

	std::uint32_t index = kInvalidIndex;
	std::uint32_t generation = 0;

	[[nodiscard]] constexpr bool operator==(const TaskHandle& other) const noexcept {
		return index == other.index && generation == other.generation;
	}
	[[nodiscard]] constexpr bool isValid() const noexcept { return index != kInvalidIndex; }
};

} // namespace trivial::task

#endif // TRIVIAL_TASK_TASK_HANDLE_H

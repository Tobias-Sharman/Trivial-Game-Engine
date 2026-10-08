#ifndef TRIVIAL_TASK_TASK_STATE_H
#define TRIVIAL_TASK_TASK_STATE_H

#include <cstdint>
#include <utility>
#include <vector>

#include <trivial/core/assert.h>
#include <trivial/core/compiler.h>
#include <trivial/core/sync/latch.h>
#include <trivial/task/task_handle.h>
#include <trivial/task/task_launch_options.h>
#include <trivial/task/task_payload.h>
#include <trivial/task/task_status.h>

#define TRIVIAL_TASK_STATE_STATUS_MASK 0b111U
#define TRIVIAL_TASK_STATE_PRIORITY_MASK 0b111U
#define TRIVIAL_TASK_STATE_AFFINITY_MASK 0b111U
#define TRIVIAL_TASK_STATE_LIFETIME_MASK 0b1U

#define TRIVIAL_TASK_STATE_STATUS_SHIFT 0U
#define TRIVIAL_TASK_STATE_PRIORITY_SHIFT (TRIVIAL_TASK_STATE_STATUS_SHIFT + 3U)
#define TRIVIAL_TASK_STATE_AFFINITY_SHIFT (TRIVIAL_TASK_STATE_PRIORITY_SHIFT + 3U)
#define TRIVIAL_TASK_STATE_LIFETIME_SHIFT (TRIVIAL_TASK_STATE_AFFINITY_SHIFT + 3U)

namespace trivial::task {

// TODO: Measure size, padding, and alignment on different platforms for cache
//       optimisation, can very likely be improved since now is more of a basic
//       semantic sense (decent for cache not optimised for size which is
//       significant here)
struct TaskState {
	TaskState(TaskPayload initialPayload, const TaskLaunchOptions& options) noexcept
	    : payload(std::move(initialPayload))
	    , m_packed(pack(TaskStatus::Created, options.priority, options.affinity, options.lifetime)) {}

	~TaskState() = default;

	TaskState(const TaskState&) = delete;
	TaskState& operator=(const TaskState&) = delete;

	TaskState(TaskState&&) = delete;
	TaskState& operator=(TaskState&&) = delete;

	[[nodiscard]] TaskStatus status() const noexcept {
		return static_cast<TaskStatus>(extractBits(TRIVIAL_TASK_STATE_STATUS_SHIFT, TRIVIAL_TASK_STATE_STATUS_MASK));
	}
	void setStatus(TaskStatus value) noexcept {
		insertBits(TRIVIAL_TASK_STATE_STATUS_SHIFT, TRIVIAL_TASK_STATE_STATUS_MASK, static_cast<std::uint32_t>(value));
	}

	[[nodiscard]] TaskPriority priority() const noexcept {
		return static_cast<TaskPriority>(
		    extractBits(TRIVIAL_TASK_STATE_PRIORITY_SHIFT, TRIVIAL_TASK_STATE_PRIORITY_MASK));
	}
	void setPriority(TaskPriority value) noexcept {
		insertBits(TRIVIAL_TASK_STATE_PRIORITY_SHIFT,
		           TRIVIAL_TASK_STATE_PRIORITY_MASK,
		           static_cast<std::uint32_t>(value));
	}

	[[nodiscard]] TaskAffinity affinity() const noexcept {
		return static_cast<TaskAffinity>(
		    extractBits(TRIVIAL_TASK_STATE_AFFINITY_SHIFT, TRIVIAL_TASK_STATE_AFFINITY_MASK));
	}
	void setAffinity(TaskAffinity value) noexcept {
		insertBits(TRIVIAL_TASK_STATE_AFFINITY_SHIFT,
		           TRIVIAL_TASK_STATE_AFFINITY_MASK,
		           static_cast<std::uint32_t>(value));
	}

	[[nodiscard]] TaskLifetime lifetime() const noexcept {
		return static_cast<TaskLifetime>(
		    extractBits(TRIVIAL_TASK_STATE_LIFETIME_SHIFT, TRIVIAL_TASK_STATE_LIFETIME_MASK));
	}
	void setLifetime(TaskLifetime value) noexcept {
		insertBits(TRIVIAL_TASK_STATE_LIFETIME_SHIFT,
		           TRIVIAL_TASK_STATE_LIFETIME_MASK,
		           static_cast<std::uint32_t>(value));
	}

	TaskPayload payload;
	std::vector<TaskHandle> dependants;
	std::vector<TaskHandle> prerequisites;

	TaskScopeHandle scope = {};

	sync::Latch* latch = nullptr;

private:
	[[nodiscard]] static constexpr std::uint16_t pack(TaskStatus status,
	                                                  TaskPriority priority,
	                                                  TaskAffinity affinity,
	                                                  TaskLifetime lifetime) noexcept {
		TRIVIAL_ASSERT(static_cast<std::uint32_t>(status) <= TRIVIAL_TASK_STATE_STATUS_MASK);
		TRIVIAL_ASSERT(static_cast<std::uint32_t>(priority) <= TRIVIAL_TASK_STATE_PRIORITY_MASK);
		TRIVIAL_ASSERT(static_cast<std::uint32_t>(affinity) <= TRIVIAL_TASK_STATE_AFFINITY_MASK);
		TRIVIAL_ASSERT(static_cast<std::uint32_t>(lifetime) <= TRIVIAL_TASK_STATE_LIFETIME_MASK);

		return static_cast<std::uint16_t>(
		    (static_cast<std::uint32_t>(status) << TRIVIAL_TASK_STATE_STATUS_SHIFT)
		    | (static_cast<std::uint32_t>(priority) << TRIVIAL_TASK_STATE_PRIORITY_SHIFT)
		    | (static_cast<std::uint32_t>(affinity) << TRIVIAL_TASK_STATE_AFFINITY_SHIFT)
		    | (static_cast<std::uint32_t>(lifetime) << TRIVIAL_TASK_STATE_LIFETIME_SHIFT));
	}

	// TODO: See if there might be a better way that does not require casting to uint32_t for shift operations

	[[nodiscard]] TRIVIAL_FORCE_INLINE std::uint32_t extractBits(std::uint32_t shift,
	                                                             std::uint32_t mask) const noexcept {
		return (static_cast<std::uint32_t>(m_packed) >> shift) & mask;
	}

	TRIVIAL_FORCE_INLINE void insertBits(std::uint32_t shift, std::uint32_t mask, std::uint32_t value) noexcept {
		TRIVIAL_ASSERT(value <= mask);

		const std::uint32_t kCleared = static_cast<std::uint32_t>(m_packed) & ~(mask << shift);
		m_packed = static_cast<std::uint16_t>(kCleared | (value << shift));
	}

	// NOLINTNEXTLINE(readability-magic-numbers)
	static_assert(TRIVIAL_TASK_STATE_LIFETIME_SHIFT + 1U <= 16U, "Packed fields no longer fit in uint16_t");

	std::uint16_t m_packed = 0;
};

} // namespace trivial::task

#undef TRIVIAL_TASK_STATE_STATUS_MASK
#undef TRIVIAL_TASK_STATE_PRIORITY_MASK
#undef TRIVIAL_TASK_STATE_AFFINITY_MASK
#undef TRIVIAL_TASK_STATE_LIFETIME_MASK

#undef TRIVIAL_TASK_STATE_STATUS_SHIFT
#undef TRIVIAL_TASK_STATE_PRIORITY_SHIFT
#undef TRIVIAL_TASK_STATE_AFFINITY_SHIFT
#undef TRIVIAL_TASK_STATE_LIFETIME_SHIFT

#endif // TRIVIAL_TASK_TASK_STATE_H

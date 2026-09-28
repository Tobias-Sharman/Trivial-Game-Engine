// task_state.h
#ifndef TRIVIAL_SRC_TASK_TASK_STATE_H
#define TRIVIAL_SRC_TASK_TASK_STATE_H

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

namespace trivial::task {

// TODO: Measure size, padding, and alignment on different platforms for cache
//       optimisation, can very likely be improved since now is more of a basic
//       semantic sense (decent for cache not optimised for size which is
//       significant here)
struct TaskState {
	TaskState(TaskPayload payload, const TaskLaunchOptions& options) noexcept
	    : payload(std::move(payload))
	    , m_packed(pack(TaskStatus::Created, options.priority, options.affinity, options.lifetime)) {}

	~TaskState() = default;

	TaskState(const TaskState&) = delete;
	TaskState& operator=(const TaskState&) = delete;

	TaskState(TaskState&&) = delete;
	TaskState& operator=(TaskState&&) = delete;

	[[nodiscard]] TaskStatus status() const noexcept {
		return static_cast<TaskStatus>(extractBits(s_kStatusShift, s_kStatusMask));
	}
	void setStatus(TaskStatus value) noexcept {
		insertBits(s_kStatusShift, s_kStatusMask, static_cast<std::uint32_t>(value));
	}

	[[nodiscard]] TaskPriority priority() const noexcept {
		return static_cast<TaskPriority>(extractBits(s_kPriorityShift, s_kPriorityMask));
	}
	void setPriority(TaskPriority value) noexcept {
		insertBits(s_kPriorityShift, s_kPriorityMask, static_cast<std::uint32_t>(value));
	}

	[[nodiscard]] TaskAffinity affinity() const noexcept {
		return static_cast<TaskAffinity>(extractBits(s_kAffinityShift, s_kAffinityMask));
	}
	void setAffinity(TaskAffinity value) noexcept {
		insertBits(s_kAffinityShift, s_kAffinityMask, static_cast<std::uint32_t>(value));
	}

	[[nodiscard]] TaskLifetime lifetime() const noexcept {
		return static_cast<TaskLifetime>(extractBits(s_kLifetimeShift, s_kLifetimeMask));
	}
	void setLifetime(TaskLifetime value) noexcept {
		insertBits(s_kLifetimeShift, s_kLifetimeMask, static_cast<std::uint32_t>(value));
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
		TRIVIAL_ASSERT(static_cast<std::uint32_t>(status) <= s_kStatusMask);
		TRIVIAL_ASSERT(static_cast<std::uint32_t>(priority) <= s_kPriorityMask);
		TRIVIAL_ASSERT(static_cast<std::uint32_t>(affinity) <= s_kAffinityMask);
		TRIVIAL_ASSERT(static_cast<std::uint32_t>(lifetime) <= s_kLifetimeMask);

		return static_cast<std::uint16_t>((static_cast<std::uint32_t>(status) << s_kStatusShift)
		                                  | (static_cast<std::uint32_t>(priority) << s_kPriorityShift)
		                                  | (static_cast<std::uint32_t>(affinity) << s_kAffinityShift)
		                                  | (static_cast<std::uint32_t>(lifetime) << s_kLifetimeShift));
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE std::uint32_t extractBits(std::uint32_t shift,
	                                                             std::uint32_t mask) const noexcept {
		return (static_cast<std::uint32_t>(m_packed) >> shift) & mask;
	}

	TRIVIAL_FORCE_INLINE void insertBits(std::uint32_t shift, std::uint32_t mask, std::uint32_t value) noexcept {
		TRIVIAL_ASSERT(value <= mask);

		const std::uint32_t kCleared = static_cast<std::uint32_t>(m_packed) & ~(mask << shift);
		m_packed = static_cast<std::uint16_t>(kCleared | (value << shift));
	}

	static constexpr std::uint32_t s_kStatusMask = 0b111U;
	static constexpr std::uint32_t s_kPriorityMask = 0b111U;
	static constexpr std::uint32_t s_kAffinityMask = 0b111U;
	static constexpr std::uint32_t s_kLifetimeMask = 0b1U;

	static constexpr std::uint32_t s_kStatusShift = 0U;
	static constexpr std::uint32_t s_kPriorityShift = s_kStatusShift + 3U;
	static constexpr std::uint32_t s_kAffinityShift = s_kPriorityShift + 3U;
	static constexpr std::uint32_t s_kLifetimeShift = s_kAffinityShift + 3U;

	// NOLINTNEXTLINE(readability-magic-numbers)
	static_assert(s_kLifetimeShift + 1U <= 16U, "Packed fields no longer fit in uint16_t");

	std::uint16_t m_packed = 0;
};

} // namespace trivial::task

#endif // TRIVIAL_SRC_TASK_TASK_STATE_H

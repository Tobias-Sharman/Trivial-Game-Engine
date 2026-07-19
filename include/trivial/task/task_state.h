// task_state.h
#ifndef TRIVIAL_SRC_TASK_TASK_STATE_H
#define TRIVIAL_SRC_TASK_TASK_STATE_H

#include <cstdint>
#include <utility>
#include <vector>

#include <trivial/core/assert.h>
#include <trivial/task/task_handle.h>
#include <trivial/task/task_launch_options.h>
#include <trivial/task/task_payload.h>
#include <trivial/task/task_status.h>
#include <trivial/task/task_wait_group.h>

namespace trivial::task {

// TODO: Measure size, padding, and alignment on different platforms for cache optimisation, can very likely be improved
//       since now is more of a basic semantic sense (decenet for cache not optimised for size which is significant
//       here)
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
		return static_cast<TaskStatus>((m_packed >> kStatusShift) & kStatusMask);
	}
	void setStatus(TaskStatus value) noexcept {
		TRIVIAL_ASSERT(static_cast<std::uint16_t>(value) <= kStatusMask);
		m_packed = static_cast<std::uint16_t>((m_packed & ~(kStatusMask << kStatusShift))
		                                      | (static_cast<std::uint16_t>(value) << kStatusShift));
	}

	[[nodiscard]] TaskPriority priority() const noexcept {
		return static_cast<TaskPriority>((m_packed >> kPriorityShift) & kPriorityMask);
	}
	void setPriority(TaskPriority value) noexcept {
		TRIVIAL_ASSERT(static_cast<std::uint16_t>(value) <= kPriorityMask);
		m_packed = static_cast<std::uint16_t>((m_packed & ~(kPriorityMask << kPriorityShift))
		                                      | (static_cast<std::uint16_t>(value) << kPriorityShift));
	}

	[[nodiscard]] TaskAffinity affinity() const noexcept {
		return static_cast<TaskAffinity>((m_packed >> kAffinityShift) & kAffinityMask);
	}
	void setAffinity(TaskAffinity value) noexcept {
		TRIVIAL_ASSERT(static_cast<std::uint16_t>(value) <= kAffinityMask);
		m_packed = static_cast<std::uint16_t>((m_packed & ~(kAffinityMask << kAffinityShift))
		                                      | (static_cast<std::uint16_t>(value) << kAffinityShift));
	}

	[[nodiscard]] TaskLifetime lifetime() const noexcept {
		return static_cast<TaskLifetime>((m_packed >> kLifetimeShift) & kLifetimeMask);
	}
	void setLifetime(TaskLifetime value) noexcept {
		TRIVIAL_ASSERT(static_cast<std::uint16_t>(value) <= kLifetimeMask);
		m_packed = static_cast<std::uint16_t>((m_packed & ~(kLifetimeMask << kLifetimeShift))
		                                      | (static_cast<std::uint16_t>(value) << kLifetimeShift));
	}

	TaskPayload payload;
	std::vector<TaskHandle> dependants;
	std::vector<TaskHandle> prerequisites;

	TaskScopeHandle scope = {};

	TaskWaitGroup* waitGroup = nullptr;

private:
	[[nodiscard]] static constexpr std::uint16_t pack(TaskStatus status,
	                                                  TaskPriority priority,
	                                                  TaskAffinity affinity,
	                                                  TaskLifetime lifetime) noexcept {
		TRIVIAL_ASSERT(static_cast<std::uint16_t>(status) <= kStatusMask);
		TRIVIAL_ASSERT(static_cast<std::uint16_t>(priority) <= kPriorityMask);
		TRIVIAL_ASSERT(static_cast<std::uint16_t>(affinity) <= kAffinityMask);
		TRIVIAL_ASSERT(static_cast<std::uint16_t>(lifetime) <= kLifetimeMask);

		return static_cast<std::uint16_t>((static_cast<std::uint16_t>(status) << kStatusShift)
		                                  | (static_cast<std::uint16_t>(priority) << kPriorityShift)
		                                  | (static_cast<std::uint16_t>(affinity) << kAffinityShift)
		                                  | (static_cast<std::uint16_t>(lifetime) << kLifetimeShift));
	}

	static constexpr std::uint16_t kStatusMask = 0b111;
	static constexpr std::uint16_t kPriorityMask = 0b111;
	static constexpr std::uint16_t kAffinityMask = 0b111;
	static constexpr std::uint16_t kLifetimeMask = 0b1;

	static constexpr std::uint16_t kStatusShift = 0;
	static constexpr std::uint16_t kPriorityShift = kStatusShift + 3;
	static constexpr std::uint16_t kAffinityShift = kPriorityShift + 3;
	static constexpr std::uint16_t kLifetimeShift = kAffinityShift + 3;

	static_assert(kLifetimeShift + 1 <= 16, "Packed fields no longer fit in uint16_t");

	std::uint16_t m_packed = 0;
};

} // namespace trivial::task

#endif // TRIVIAL_SRC_TASK_TASK_STATE_H

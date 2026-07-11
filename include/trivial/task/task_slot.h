#ifndef TRIVIAL_TASK_TASK_SLOT_H
#define TRIVIAL_TASK_TASK_SLOT_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <utility>

#include <trivial/core/assert.h>
#include <trivial/task/task_handle.h>
#include <trivial/task/task_mutex.h>
#include <trivial/task/task_payload.h>
#include <trivial/task/task_state.h>

namespace trivial::task {

enum class TaskSlotState : std::uint8_t {
	Free,
	Occupied
};

class TaskSlot { // NOLINT(cppcoreguidelines-pro-type-member-init)
public:
	TaskSlot() noexcept = default;

	~TaskSlot() noexcept {
		if (isOccupied()) {
			std::destroy_at(statePointer());
		}
	}

	TaskSlot(const TaskSlot&) = delete;
	TaskSlot& operator=(const TaskSlot&) = delete;

	TaskSlot(TaskSlot&&) = delete;
	TaskSlot& operator=(TaskSlot&&) = delete;

	[[nodiscard]] TaskSlotMutex& mutex() const noexcept { return m_mutex; }

	[[nodiscard]] bool isOccupied() const noexcept { return m_state == TaskSlotState::Occupied; }

	[[nodiscard]] std::uint32_t generation() const noexcept { return m_generation; }

	[[nodiscard]] bool isOccupiedBy(TaskHandle taskHandle) const noexcept {
		return isOccupied() && taskHandle.generation == m_generation;
	}

	[[nodiscard]] TaskState& state() noexcept {
		TRIVIAL_ASSERT(isOccupied());

		return *statePointer();
	}
	[[nodiscard]] const TaskState& state() const noexcept {
		TRIVIAL_ASSERT(isOccupied());

		return *statePointer();
	}

	TaskState& construct(TaskPayload payload, const TaskLaunchOptions& options) noexcept {
		TRIVIAL_ASSERT(!isOccupied());
		TRIVIAL_ASSERT(options.priority < TaskPriority::Count);

		TaskState* state = std::construct_at(rawStatePointer(), std::move(payload), options);

		m_state = TaskSlotState::Occupied;

		return *state;
	}

	void destroy() noexcept {
		TRIVIAL_ASSERT(isOccupied());

		std::destroy_at(statePointer());

		m_state = TaskSlotState::Free;
		++m_generation;
		// NOTE: In theory an old handle could alias a new lifetime but that would be beyond unlikely as would require
		//       roughly 4.3 billion retirements without the old handle just being removed. Can just add some check
		//       later if somehow this amount is reached. Going for a day of constant running about 50,000 reuses per
		//       second. So technically possible but if you are keeping a handle for a task that was retired that long
		//       that is bad design foremostly
	}

private:
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	[[nodiscard]] TaskState* rawStatePointer() noexcept { return reinterpret_cast<TaskState*>(m_storage.data()); }
	[[nodiscard]] const TaskState* rawStatePointer() const noexcept {
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		return reinterpret_cast<const TaskState*>(m_storage.data());
	}

	[[nodiscard]] TaskState* statePointer() noexcept { return std::launder(rawStatePointer()); }
	[[nodiscard]] const TaskState* statePointer() const noexcept { return std::launder(rawStatePointer()); }

	mutable TaskSlotMutex m_mutex;

	alignas(TaskState) std::array<std::byte, sizeof(TaskState)> m_storage;

	std::uint32_t m_generation = 0;
	TaskSlotState m_state = TaskSlotState::Free;
};

} // namespace trivial::task

#endif // TRIVIAL_TASK_TASK_SLOT_H

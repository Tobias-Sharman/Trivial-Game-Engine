#ifndef TRIVIAL_SRC_CORE_SYNC_PARKING_LOT_H
#define TRIVIAL_SRC_CORE_SYNC_PARKING_LOT_H

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <utility>

#include <trivial/core/assert.h>
#include <trivial/core/thread/thread.h>

#include "core/sync/parking_lot_slot.h"

// NOTE: For later implementation on systems with a known fixed thread count
//       the global could be dropped for a static version. A lazy initialisation
//       singleton is not pursued, since the gain from one less pointer
//       indirection is insignificant compared to the rest of the function and
//       will make the behaviour less clean and consistent. The global will
//       likely be in lower cache level so fetching is quick anyway

// TODO: Make and test against a hash table, linked list backed parking lot
//         - Delayed since walking an array is expected to be faster than
//           pointer chasing for the number of threads on systems for gaming
//         - Compile time gated
//             * have some 'using ParkingLot = ParkingLotArrayBacked' style for
//               nicer API, will need to see about unparkAll for its return if
//               the amount of awoken really matters for the optimisations
//               Amaneiu's parking lot in Rust makes

namespace trivial::sync {

class ParkingLot {
public:
	enum class ParkResult : std::uint8_t {
		Unparked,
		Invalidated,
		TimedOut,
	};

	explicit ParkingLot(std::size_t capacity) noexcept
	    : m_slots(capacity) {}

	~ParkingLot() noexcept = default;

	ParkingLot(const ParkingLot&) = delete;
	ParkingLot& operator=(const ParkingLot&) = delete;

	ParkingLot(ParkingLot&&) = delete;
	ParkingLot& operator=(ParkingLot&&) = delete;

	template <typename Validate>
	[[nodiscard]] ParkResult park(std::uintptr_t address, Validate&& validate) noexcept {
		ParkingLotSlot& slot = slotForCurrentThread();
		TRIVIAL_ASSERT(slot.key.load(std::memory_order_relaxed) == 0);
		slot.parker.prepare();
		slot.key.store(address, std::memory_order_release);

		if (!std::forward<Validate>(validate)()) {
			slot.key.store(0, std::memory_order_relaxed);
			return ParkResult::Invalidated;
		}

		slot.parker.park();
		slot.key.store(0, std::memory_order_relaxed);
		return ParkResult::Unparked;
	}

	template <typename Validate, typename BeforeSleep>
	[[nodiscard]] ParkResult park(std::uintptr_t address, Validate&& validate, BeforeSleep&& beforeSleep) noexcept {
		ParkingLotSlot& slot = slotForCurrentThread();
		TRIVIAL_ASSERT(slot.key.load(std::memory_order_relaxed) == 0);
		slot.parker.prepare();
		slot.key.store(address, std::memory_order_release);

		if (!std::forward<Validate>(validate)()) {
			slot.key.store(0, std::memory_order_relaxed);
			return ParkResult::Invalidated;
		}

		std::forward<BeforeSleep>(beforeSleep)();
		slot.parker.park();
		slot.key.store(0, std::memory_order_relaxed);
		return ParkResult::Unparked;
	}

	template <typename Validate>
	[[nodiscard]] ParkResult parkFor(std::uintptr_t address,
	                                 std::chrono::nanoseconds timeout,
	                                 Validate&& validate) noexcept {
		ParkingLotSlot& slot = slotForCurrentThread();
		TRIVIAL_ASSERT(slot.key.load(std::memory_order_relaxed) == 0);
		slot.parker.prepare();
		slot.key.store(address, std::memory_order_release);

		if (!std::forward<Validate>(validate)()) {
			slot.key.store(0, std::memory_order_relaxed);
			return ParkResult::Invalidated;
		}

		if (slot.parker.parkFor(timeout)) {
			slot.key.store(0, std::memory_order_relaxed);
			return ParkResult::Unparked;
		}

		slot.key.store(0, std::memory_order_relaxed);
		return ParkResult::TimedOut;
	}

	template <typename Validate, typename BeforeSleep>
	[[nodiscard]] ParkResult parkFor(std::uintptr_t address,
	                                 std::chrono::nanoseconds timeout,
	                                 Validate&& validate,
	                                 BeforeSleep&& beforeSleep) noexcept {
		ParkingLotSlot& slot = slotForCurrentThread();
		TRIVIAL_ASSERT(slot.key.load(std::memory_order_relaxed) == 0);
		slot.parker.prepare();
		slot.key.store(address, std::memory_order_release);

		if (!std::forward<Validate>(validate)()) {
			slot.key.store(0, std::memory_order_relaxed);
			return ParkResult::Invalidated;
		}

		std::forward<BeforeSleep>(beforeSleep)();

		if (slot.parker.parkFor(timeout)) {
			slot.key.store(0, std::memory_order_relaxed);
			return ParkResult::Unparked;
		}

		slot.key.store(0, std::memory_order_relaxed);
		return ParkResult::TimedOut;
	}

	[[nodiscard]] bool unparkOne(std::uintptr_t address) noexcept {
		for (std::size_t index = 0; index < m_slots.size(); ++index) {
			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
			ParkingLotSlot& slot = m_slots[index];

			if (slot.key.load(std::memory_order_relaxed) != address) {
				continue;
			}

			std::uintptr_t expected = address;
			if (!slot.key.compare_exchange_strong(expected, 0, std::memory_order_acquire, std::memory_order_relaxed)) {
				continue;
			}

			slot.parker.beginUnpark().wake();
			return true;
		}

		return false;
	}

	void unparkAll(std::uintptr_t address) noexcept {
		for (std::size_t index = 0; index < m_slots.size(); ++index) {
			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
			ParkingLotSlot& slot = m_slots[index];

			if (slot.key.load(std::memory_order_relaxed) != address) {
				continue;
			}

			std::uintptr_t expected = address;
			if (!slot.key.compare_exchange_strong(expected, 0, std::memory_order_acquire, std::memory_order_relaxed)) {
				continue;
			}

			slot.parker.beginUnpark().wake();
		}
	}

private:
	[[nodiscard]] ParkingLotSlot& slotForCurrentThread() noexcept {
		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
		return m_slots[trivial::thread::Thread::current()->index()];
	}

	ParkingLotSlotArray m_slots;
};

namespace detail {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
inline ParkingLot* g_activeParkingLot = nullptr;

} // namespace detail

inline void setActiveParkingLot(ParkingLot* parkingLot) noexcept {
	detail::g_activeParkingLot = parkingLot;
}

[[nodiscard]] inline ParkingLot& activeParkingLot() noexcept {
	TRIVIAL_ASSERT(detail::g_activeParkingLot != nullptr);
	return *detail::g_activeParkingLot;
}

} // namespace trivial::sync

#endif // TRIVIAL_SRC_CORE_SYNC_PARKING_LOT_H

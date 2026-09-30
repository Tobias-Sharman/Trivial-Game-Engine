#define TRIVIAL_SYNC_MUTEX_IMPLEMENTATION
#include <trivial/core/sync/mutex.h>

#include <atomic>
#include <cstdint>

#include <trivial/core/sync/spin_wait.h>
#include <trivial/core/sync/sync_config.h>

#include "core/sync/parking_lot.h"

namespace trivial::sync {

void Mutex::lockSlow() noexcept {
	std::uint32_t spinCount = 0;
	std::uint8_t state = m_state.load(std::memory_order_relaxed);

	for (;;) {
		if ((state & TRIVIAL_SYNC_MUTEX_LOCKED_BIT) == 0) {
			if (m_state.compare_exchange_weak(state,
			                                  static_cast<std::uint8_t>(state | TRIVIAL_SYNC_MUTEX_LOCKED_BIT),
			                                  std::memory_order_acquire,
			                                  std::memory_order_relaxed)) {
				return;
			}

			continue;
		}

		if ((state & TRIVIAL_SYNC_MUTEX_PARKED_BIT) == 0 && spinCount < TRIVIAL_SYNC_MAX_SPIN_COUNT) {
			spinWaitForever(spinCount);
			state = m_state.load(std::memory_order_relaxed);
			continue;
		}

		if (((state & TRIVIAL_SYNC_MUTEX_PARKED_BIT) == 0)
		    && (!m_state.compare_exchange_weak(state,
		                                       static_cast<std::uint8_t>(state | TRIVIAL_SYNC_MUTEX_PARKED_BIT),
		                                       std::memory_order_relaxed,
		                                       std::memory_order_relaxed))) {
			continue;
		}

		(void)activeParkingLot().park(parkingKey(*this), [this] {
			return m_state.load(std::memory_order_relaxed)
			       == (TRIVIAL_SYNC_MUTEX_LOCKED_BIT | TRIVIAL_SYNC_MUTEX_PARKED_BIT);
		});

		spinCount = 0;
		state = m_state.load(std::memory_order_relaxed);
	}
}

void Mutex::unlockSlow() noexcept {
	activeParkingLot().unparkOne(parkingKey(*this), [this](ParkingLot::UnparkOneResult result) {
		if (result.hasMoreWaiters) {
			m_state.store(TRIVIAL_SYNC_MUTEX_PARKED_BIT, std::memory_order_release);
		} else {
			m_state.store(0, std::memory_order_release);
		}
	});
}

} // namespace trivial::sync

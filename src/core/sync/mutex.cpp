#include <trivial/core/sync/mutex.h>

#include <atomic>
#include <cstdint>

#include <trivial/core/sync/spin_wait.h>
#include <trivial/core/sync/sync_config.h>

#include "core/sync/parking_lot.h"

namespace {

[[nodiscard]] std::uintptr_t keyFor(const trivial::sync::Mutex* const kMutex) noexcept {
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	return reinterpret_cast<std::uintptr_t>(kMutex);
}

} // namespace

namespace trivial::sync {

void Mutex::lockSlow() noexcept {
	std::uint32_t spinCount = 0;
	std::uint8_t state = m_state.load(std::memory_order_relaxed);

	for (;;) {
		if ((state & kLockedBit) == 0) {
			if (m_state.compare_exchange_weak(state,
			                                  static_cast<std::uint8_t>(state | kLockedBit),
			                                  std::memory_order_acquire,
			                                  std::memory_order_relaxed)) {
				return;
			}

			continue;
		}

		if ((state & kParkedBit) == 0 && spinCount < TRIVIAL_SYNC_MAX_SPIN_COUNT) {
			spinWaitForever(spinCount);
			state = m_state.load(std::memory_order_relaxed);
			continue;
		}

		if ((state & kParkedBit) == 0) {
			if (!m_state.compare_exchange_weak(state,
			                                   static_cast<std::uint8_t>(state | kParkedBit),
			                                   std::memory_order_relaxed,
			                                   std::memory_order_relaxed)) {
				continue;
			}
		}

		(void)activeParkingLot().park(keyFor(this), [this] {
			return m_state.load(std::memory_order_relaxed) == (kLockedBit | kParkedBit);
		});

		spinCount = 0;
		state = m_state.load(std::memory_order_relaxed);
	}
}

void Mutex::unlockSlow() noexcept {
	activeParkingLot().unparkOne(keyFor(this), [this](const ParkingLot::UnparkOneResult kResult) {
		if (kResult.hasMoreWaiters) {
			m_state.store(kParkedBit, std::memory_order_release);
		} else {
			m_state.store(0, std::memory_order_release);
		}
	});
}

} // namespace trivial::sync

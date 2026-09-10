#include <trivial/core/sync/mutex.h>

#include <atomic>
#include <cstdint>

#include <trivial/core/sync/spin_wait.h>
#include <trivial/core/sync/sync_config.h>

#include "core/sync/parking_lot.h"

namespace {

[[nodiscard]] std::uintptr_t keyFor(const trivial::sync::Mutex* mutex) noexcept {
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	return reinterpret_cast<std::uintptr_t>(mutex);
}

} // namespace

namespace trivial::sync {

void Mutex::lockSlow() noexcept {
	std::uint32_t spinCount = 0;
	std::uint8_t state = m_state.load(std::memory_order_relaxed);

	for (;;) {
		if (state == kLockedUncontended && spinCount < TRIVIAL_SYNC_MAX_SPIN_COUNT) {
			spinWaitForever(spinCount);
			state = m_state.load(std::memory_order_relaxed);
			continue;
		}

		state = m_state.exchange(kLockedContended, std::memory_order_acquire);

		if (state == kUnlocked) {
			return;
		}

		(void)activeParkingLot().park(keyFor(this), [this] {
			return m_state.load(std::memory_order_relaxed) == kLockedContended;
		});

		spinCount = 0;
		state = m_state.load(std::memory_order_relaxed);
	}
}

void Mutex::unlockSlow() noexcept {
	m_state.store(kUnlocked, std::memory_order_release);
	(void)activeParkingLot().unparkOne(keyFor(this));
}

} // namespace trivial::sync

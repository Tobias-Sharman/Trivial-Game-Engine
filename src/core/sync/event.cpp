#include <trivial/core/sync/event.h>

#include <atomic>

#include <trivial/core/time/duration.h>
#include <trivial/core/time/instant.h>
#include <trivial/core/time/time.h>

#include "core/sync/parking_lot.h"

namespace trivial::sync {

void Event::wait() noexcept {
	while (!isTriggered()) {
		(void)activeParkingLot().park(parkingKey(*this), [this] {
			return !isTriggered();
		});
	}
}

[[nodiscard]] bool Event::waitFor(time::Duration timeout) noexcept {
	const time::Instant kExpiry = time::now().clampedAdd(timeout);

	while (!isTriggered()) {
		const time::Instant kNow = time::now();
		if (kExpiry <= kNow) {
			return isTriggered();
		}

		const ParkingLot::ParkResult kResult = activeParkingLot().parkFor(parkingKey(*this), kExpiry - kNow, [this] {
			return !isTriggered();
		});

		if (kResult == ParkingLot::ParkResult::TimedOut) {
			return isTriggered();
		}
	}

	return true;
}

void Event::trigger() noexcept {
	m_isTriggered.store(true, std::memory_order_release);
	activeParkingLot().unparkAll(parkingKey(*this));
}

} // namespace trivial::sync

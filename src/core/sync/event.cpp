#include <trivial/core/sync/event.h>

#include <atomic>
#include <chrono>
#include <cstdint>

#include "core/sync/parking_lot.h"

namespace {

[[nodiscard]] std::uintptr_t keyFor(const trivial::sync::Event* event) noexcept {
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	return reinterpret_cast<std::uintptr_t>(event);
}

} // namespace

namespace trivial::sync {

void Event::wait() noexcept {
	while (!isTriggered()) {
		(void)activeParkingLot().park(keyFor(this), [this] {
			return !isTriggered();
		});
	}
}

[[nodiscard]] bool Event::waitFor(std::chrono::nanoseconds timeout) noexcept {
	const std::chrono::steady_clock::time_point kExpiry = std::chrono::steady_clock::now() + timeout;

	while (!isTriggered()) {
		const std::chrono::steady_clock::time_point kNow = std::chrono::steady_clock::now();
		if (kExpiry <= kNow) {
			return isTriggered();
		}

		const ParkingLot::ParkResult kResult = activeParkingLot().parkFor(keyFor(this), kExpiry - kNow, [this] {
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
	activeParkingLot().unparkAll(keyFor(this));
}

} // namespace trivial::sync

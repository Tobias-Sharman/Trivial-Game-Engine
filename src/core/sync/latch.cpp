#include <trivial/core/sync/latch.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <stop_token>

#include <trivial/core/assert.h>

#include "core/sync/parking_lot.h"

namespace {

[[nodiscard]] std::uintptr_t keyFor(const trivial::sync::Latch* latch) noexcept {
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	return reinterpret_cast<std::uintptr_t>(latch);
}

} // namespace

namespace trivial::sync {

void Latch::wait() noexcept {
	while (remaining() > 0) {
		(void)activeParkingLot().park(keyFor(this), [this] {
			return remaining() > 0;
		});
	}
}

[[nodiscard]] bool Latch::wait(const std::stop_token& stopToken) noexcept {
	if (stopToken.stop_requested()) {
		return false;
	}

	std::stop_callback cancelCallback(stopToken, [this] {
		activeParkingLot().unparkAll(keyFor(this));
	});

	while (remaining() > 0) {
		if (stopToken.stop_requested()) {
			return false;
		}

		(void)activeParkingLot().park(keyFor(this), [this, &stopToken] {
			return remaining() > 0 && !stopToken.stop_requested();
		});
	}

	return true;
}

void Latch::countDown() noexcept {
	const std::size_t kPrevious = m_remaining.fetch_sub(1, std::memory_order_acq_rel);
	TRIVIAL_ASSERT(kPrevious > 0);

	if (kPrevious == 1) {
		activeParkingLot().unparkAll(keyFor(this));
	}
}

} // namespace trivial::sync

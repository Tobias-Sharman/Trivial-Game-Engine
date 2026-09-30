#include <trivial/core/sync/latch.h>

#include <atomic>
#include <cstddef>

#include <trivial/core/assert.h>

#include "core/sync/parking_lot.h"

namespace trivial::sync {

void Latch::wait() noexcept {
	while (remaining() > 0) {
		(void)activeParkingLot().park(parkingKey(*this), [this] {
			return remaining() > 0;
		});
	}
}

void Latch::countDown() noexcept {
	const std::size_t kPrevious = m_remaining.fetch_sub(1, std::memory_order_acq_rel);
	TRIVIAL_ASSERT(kPrevious > 0);

	if (kPrevious == 1) {
		activeParkingLot().unparkAll(parkingKey(*this));
	}
}

} // namespace trivial::sync

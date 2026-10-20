#include <trivial/core/sync/semaphore.h>

#include <atomic>

#include "core/sync/parking_lot.h"

namespace trivial::sync {

void Semaphore::acquire() noexcept {
	while (!tryAcquire()) {
		(void)activeParkingLot().park(parkingKey(*this), [this] {
			return m_count.load(std::memory_order_relaxed) == 0;
		});
	}
}

void Semaphore::release() noexcept {
	m_count.fetch_add(1, std::memory_order_release);
	(void)activeParkingLot().unparkOne(parkingKey(*this));
}

} // namespace trivial::sync

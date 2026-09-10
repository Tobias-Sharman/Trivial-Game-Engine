#include <trivial/core/sync/semaphore.h>

#include <atomic>
#include <cstddef>
#include <cstdint>

#include "core/sync/parking_lot.h"

namespace {

[[nodiscard]] std::uintptr_t keyFor(const trivial::sync::Semaphore* semaphore) noexcept {
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	return reinterpret_cast<std::uintptr_t>(semaphore);
}

} // namespace

namespace trivial::sync {

void Semaphore::acquire() noexcept {
	while (!tryAcquire()) {
		(void)activeParkingLot().park(keyFor(this), [this] {
			return m_count.load(std::memory_order_relaxed) == 0;
		});
	}
}

void Semaphore::release() noexcept {
	m_count.fetch_add(1, std::memory_order_release);
	(void)activeParkingLot().unparkOne(keyFor(this));
}

} // namespace trivial::sync

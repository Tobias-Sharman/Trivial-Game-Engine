#include "core/sync/parker.h"

#include <atomic>
#include <cerrno>
#include <cstdint>
#include <os/clock.h>
#include <os/os_sync_wait_on_address.h>

#include <trivial/core/assert.h>
#include <trivial/core/time/duration.h>
#include <trivial/core/time/instant.h>
#include <trivial/core/time/time.h>

namespace {

void osSyncWait(std::atomic<std::uint32_t>& state) noexcept {
	const int kResult = os_sync_wait_on_address(&state, 1, sizeof(std::uint32_t), OS_SYNC_WAIT_ON_ADDRESS_NONE);
	if (kResult == -1) {
		TRIVIAL_ASSERT(errno == EINTR || errno == EFAULT); // NOLINT(readability-simplify-boolean-expr)
	}
	(void)kResult;
}

void osSyncWaitFor(std::atomic<std::uint32_t>& state, trivial::time::Duration timeout) noexcept {
	const int kResult = os_sync_wait_on_address_with_timeout(&state,
	                                                         1,
	                                                         sizeof(std::uint32_t),
	                                                         OS_SYNC_WAIT_ON_ADDRESS_NONE,
	                                                         OS_CLOCK_MACH_ABSOLUTE_TIME,
	                                                         static_cast<std::uint64_t>(timeout.count()));
	if (kResult == -1) {
		// NOLINTNEXTLINE(readability-simplify-boolean-expr)
		TRIVIAL_ASSERT(errno == EINTR || errno == EFAULT || errno == ETIMEDOUT);
	}
	(void)kResult;
}

void osSyncWake(std::atomic<std::uint32_t>& state) noexcept {
	const int kResult = os_sync_wake_by_address_any(&state, sizeof(std::uint32_t), OS_SYNC_WAKE_BY_ADDRESS_NONE);
	if (kResult == -1) {
		TRIVIAL_ASSERT(errno == ENOENT);
	}
	(void)kResult;
}

} // namespace

namespace trivial::sync {

void Parker::wake() noexcept {
	osSyncWake(m_state);
}

void Parker::park() noexcept {
	while (m_state.load(std::memory_order_acquire) != 0) {
		osSyncWait(m_state);
	}
}

// os_sync_wait_on_address_with_deadline could wait on one mach tick deadline
// across spurious wakes, but it needs a Duration to ticks conversion and would
// be the only absolute deadline wait, so the relative timeout matches the
// Windows and Linux waits instead
[[nodiscard]] bool Parker::parkFor(time::Duration timeout) noexcept {
	const time::Instant kExpiry = time::now().clampedAdd(timeout);

	while (m_state.load(std::memory_order_acquire) != 0) {
		const time::Instant kNow = time::now();
		if (kExpiry <= kNow) {
			return false;
		}

		osSyncWaitFor(m_state, kExpiry - kNow);
	}

	return true;
}

} // namespace trivial::sync

#include "core/sync/parker.h"

#include <atomic>
#include <cstdint>
#include <limits>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX

#include <windows.h> // IWYU pragma: keep

#include <trivial/core/assert.h>
#include <trivial/core/time/duration.h>
#include <trivial/core/time/instant.h>
#include <trivial/core/time/time.h>

namespace {

[[nodiscard]] DWORD timeoutMilliseconds(trivial::time::Duration remaining) noexcept {
	const std::int64_t kRemainingMs = remaining.toMillisecondsCeil();

	constexpr std::int64_t kMaxTimeoutMs{std::numeric_limits<DWORD>::max()};

	if (kRemainingMs > kMaxTimeoutMs) {
		return INFINITE;
	}

	return static_cast<DWORD>(kRemainingMs);
}

} // namespace

namespace trivial::sync {

void Parker::wake() noexcept {
	WakeByAddressSingle(&m_state);
}

void Parker::park() noexcept {
	std::uint32_t compare = 1;
	while (m_state.load(std::memory_order_acquire) != 0) {
		TRIVIAL_VERIFY(WaitOnAddress(&m_state, &compare, sizeof(compare), INFINITE) != 0);
	}
}

[[nodiscard]] bool Parker::parkFor(time::Duration timeout) noexcept {
	const time::Instant kExpiry = time::now().clampedAdd(timeout);
	std::uint32_t compare = 1;

	while (m_state.load(std::memory_order_acquire) != 0) {
		const time::Instant kNow = time::now();
		if (kExpiry <= kNow) {
			return false;
		}

		const DWORD kTimeoutMs = timeoutMilliseconds(kExpiry - kNow);

		if (WaitOnAddress(&m_state, &compare, sizeof(compare), kTimeoutMs) == 0) {
			TRIVIAL_ASSERT(GetLastError() == ERROR_TIMEOUT);
		}
	}

	return true;
}

} // namespace trivial::sync

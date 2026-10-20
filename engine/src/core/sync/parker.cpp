#include "core/sync/parker.h"

#include <atomic>
#include <cstdint>

#include <trivial/core/assert.h>
#include <trivial/core/platform.h>
#include <trivial/core/time/duration.h>
#include <trivial/core/time/instant.h>
#include <trivial/core/time/time.h>

#if TRIVIAL_PLATFORM_WINDOWS
#include <limits>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX

#include <windows.h> // IWYU pragma: keep

#elif TRIVIAL_PLATFORM_LINUX
#include <cerrno>
#include <ctime>
#include <limits>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <trivial/core/time/time_constants.h>

#elif TRIVIAL_PLATFORM_MACOS
#include <cerrno>
#include <os/clock.h>
#include <os/os_sync_wait_on_address.h>

#endif // Platform-specific headers

#if TRIVIAL_PLATFORM_LINUX

namespace {

long futexSyscall(const std::atomic<std::uint32_t>* state,
                  int futexOp,
                  std::uint32_t value,
                  const timespec* timeout) noexcept {
	return syscall(SYS_futex, state, futexOp, value, timeout); // NOLINT(cppcoreguidelines-pro-type-vararg)
}

void futexWait(const std::atomic<std::uint32_t>& state, const timespec* timeout) noexcept {
	const long kResult = futexSyscall(&state, FUTEX_WAIT | FUTEX_PRIVATE_FLAG, 1, timeout);
	TRIVIAL_ASSERT(kResult == 0 || kResult == -1); // NOLINT(readability-simplify-boolean-expr)
	if (kResult == -1) {
		// NOLINTNEXTLINE(readability-simplify-boolean-expr)
		TRIVIAL_ASSERT(errno == EINTR || errno == EAGAIN || (timeout != nullptr && errno == ETIMEDOUT));
	}
	(void)kResult;
}

void futexWake(const std::atomic<std::uint32_t>& state) noexcept {
	const long kResult = futexSyscall(&state, FUTEX_WAKE | FUTEX_PRIVATE_FLAG, 1, nullptr);
	TRIVIAL_ASSERT(kResult == 0 || kResult == 1 || kResult == -1); // NOLINT(readability-simplify-boolean-expr)
	if (kResult == -1) {
		TRIVIAL_ASSERT(errno == EFAULT);
	}
	(void)kResult;
}

} // namespace

#elif TRIVIAL_PLATFORM_WINDOWS

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

#elif TRIVIAL_PLATFORM_MACOS

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

#endif // Platform-specific helpers

namespace trivial::sync {

#if TRIVIAL_PLATFORM_WINDOWS

void UnparkHandle::wake() noexcept {
	WakeByAddressSingle(&m_state->state);
}

#elif TRIVIAL_PLATFORM_LINUX

void UnparkHandle::wake() noexcept {
	futexWake(m_state->state);
}

#elif TRIVIAL_PLATFORM_MACOS

void UnparkHandle::wake() noexcept {
	osSyncWake(m_state->state);
}

#endif // Platform-specific UnparkHandle::wake

void Parker::prepare() noexcept {
	m_state.state.store(1, std::memory_order_relaxed);
}

#if TRIVIAL_PLATFORM_WINDOWS

void Parker::park() noexcept {
	std::uint32_t compare = 1;
	while (m_state.state.load(std::memory_order_acquire) != 0) {
		TRIVIAL_VERIFY(WaitOnAddress(&m_state.state, &compare, sizeof(compare), INFINITE) != 0);
	}
}

#elif TRIVIAL_PLATFORM_LINUX

void Parker::park() noexcept {
	while (m_state.state.load(std::memory_order_acquire) != 0) {
		futexWait(m_state.state, nullptr);
	}
}

#elif TRIVIAL_PLATFORM_MACOS

void Parker::park() noexcept {
	while (m_state.state.load(std::memory_order_acquire) != 0) {
		osSyncWait(m_state.state);
	}
}

#endif // Platform-specific Parker::park

#if TRIVIAL_PLATFORM_WINDOWS

[[nodiscard]] bool Parker::parkFor(time::Duration timeout) noexcept {
	const time::Instant kExpiry = time::now().clampedAdd(timeout);
	std::uint32_t compare = 1;

	while (m_state.state.load(std::memory_order_acquire) != 0) {
		const time::Instant kNow = time::now();
		if (kExpiry <= kNow) {
			return false;
		}

		const DWORD kTimeoutMs = timeoutMilliseconds(kExpiry - kNow);

		if (WaitOnAddress(&m_state.state, &compare, sizeof(compare), kTimeoutMs) == 0) {
			TRIVIAL_ASSERT(GetLastError() == ERROR_TIMEOUT);
		}
	}

	return true;
}

#elif TRIVIAL_PLATFORM_LINUX

[[nodiscard]] bool Parker::parkFor(time::Duration timeout) noexcept {
	const time::Instant kExpiry = time::now().clampedAdd(timeout);

	while (m_state.state.load(std::memory_order_acquire) != 0) {
		const time::Instant kNow = time::now();
		if (kExpiry <= kNow) {
			return false;
		}

		const time::Duration kRemaining = kExpiry - kNow;
		const std::int64_t kRemainingSec = kRemaining.toSeconds();
		if (kRemainingSec > static_cast<std::int64_t>(std::numeric_limits<std::time_t>::max())) {
			park();
			return true;
		}

		const timespec kTimeout{
		    .tv_sec = static_cast<std::time_t>(kRemainingSec),
		    .tv_nsec = static_cast<long>(kRemaining.count() % TRIVIAL_TIME_NANOSECONDS_PER_SECOND),
		};

		futexWait(m_state.state, &kTimeout);
	}

	return true;
}

#elif TRIVIAL_PLATFORM_MACOS

// os_sync_wait_on_address_with_deadline could wait on one mach tick deadline
// across spurious wakes, but it needs a Duration to ticks conversion and would
// be the only absolute deadline wait, so the relative timeout matches the
// Windows and Linux waits instead
[[nodiscard]] bool Parker::parkFor(time::Duration timeout) noexcept {
	const time::Instant kExpiry = time::now().clampedAdd(timeout);

	while (m_state.state.load(std::memory_order_acquire) != 0) {
		const time::Instant kNow = time::now();
		if (kExpiry <= kNow) {
			return false;
		}

		osSyncWaitFor(m_state.state, kExpiry - kNow);
	}

	return true;
}

#endif // Platform-specific Parker::parkFor

[[nodiscard]] UnparkHandle Parker::beginUnpark() noexcept {
	m_state.state.store(0, std::memory_order_release);
	return UnparkHandle(&m_state);
}

[[nodiscard]] bool Parker::timedOut() noexcept {
	return m_state.state.load(std::memory_order_relaxed) != 0;
}

} // namespace trivial::sync

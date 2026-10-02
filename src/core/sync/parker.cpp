#include "core/sync/parker.h"

#include <chrono> // TODO: Custom time functions
#include <limits>

#include <trivial/core/assert.h>
#include <trivial/core/platform.h>
#include <trivial/core/time/time_constants.h>

#if TRIVIAL_PLATFORM_WINDOWS
#include <atomic>
#include <cstdint>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h> // IWYU pragma: keep

#elif TRIVIAL_PLATFORM_LINUX
#include <atomic>
#include <cerrno>
#include <cstdint>
#include <ctime>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

#elif TRIVIAL_PLATFORM_MACOS
#include <cerrno>
#include <pthread.h>
#include <sys/time.h>

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

#endif // TRIVIAL_PLATFORM_LINUX

#if TRIVIAL_PLATFORM_WINDOWS

namespace {

[[nodiscard]] DWORD timeoutMilliseconds(std::chrono::nanoseconds::rep remainingNs) noexcept {
	if (remainingNs
	    > std::numeric_limits<std::chrono::nanoseconds::rep>::max() - TRIVIAL_TIME_MILLISECOND_ROUNDING_NS) {
		return INFINITE;
	}

	const std::chrono::nanoseconds::rep kRemainingMs
	    = (remainingNs + TRIVIAL_TIME_MILLISECOND_ROUNDING_NS) / TRIVIAL_TIME_NANOSECONDS_PER_MILLISECOND;

	constexpr std::chrono::nanoseconds::rep kMaxTimeoutMs{std::numeric_limits<DWORD>::max()};

	if (kRemainingMs > kMaxTimeoutMs) {
		return INFINITE;
	}

	return static_cast<DWORD>(kRemainingMs);
}

} // namespace

#endif // TRIVIAL_PLATFORM_WINDOWS

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
	m_state->notified = true;

	TRIVIAL_VERIFY(pthread_cond_signal(&m_state->condvar) == 0);
	TRIVIAL_VERIFY(pthread_mutex_unlock(&m_state->mutex) == 0);
}

#endif // Platform-specific UnparkHandle::wake

#if TRIVIAL_PLATFORM_MACOS

Parker::Parker() noexcept {
	TRIVIAL_VERIFY(pthread_mutex_init(&m_state.mutex, nullptr) == 0);
	TRIVIAL_VERIFY(pthread_cond_init(&m_state.condvar, nullptr) == 0);
}

Parker::~Parker() noexcept {
	TRIVIAL_VERIFY(pthread_cond_destroy(&m_state.condvar) == 0);
	TRIVIAL_VERIFY(pthread_mutex_destroy(&m_state.mutex) == 0);
}

#endif // TRIVIAL_PLATFORM_MACOS

#if TRIVIAL_PLATFORM_WINDOWS || TRIVIAL_PLATFORM_LINUX

void Parker::prepare() noexcept {
	m_state.state.store(1, std::memory_order_relaxed);
}

#elif TRIVIAL_PLATFORM_MACOS

void Parker::prepare() noexcept {
	TRIVIAL_VERIFY(pthread_mutex_lock(&m_state.mutex) == 0);

	m_state.notified = false;

	TRIVIAL_VERIFY(pthread_mutex_unlock(&m_state.mutex) == 0);
}

#endif // Platform-specific Parker::prepare

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
	TRIVIAL_VERIFY(pthread_mutex_lock(&m_state.mutex) == 0);

	while (!m_state.notified) {
		TRIVIAL_VERIFY(pthread_cond_wait(&m_state.condvar, &m_state.mutex) == 0);
	}

	TRIVIAL_VERIFY(pthread_mutex_unlock(&m_state.mutex) == 0);
}

#endif // Platform-specific Parker::park

#if TRIVIAL_PLATFORM_WINDOWS

[[nodiscard]] bool Parker::parkFor(std::chrono::nanoseconds lifetime) noexcept {
	const std::chrono::steady_clock::time_point kExpiry = std::chrono::steady_clock::now() + lifetime;
	std::uint32_t compare = 1;

	while (m_state.state.load(std::memory_order_acquire) != 0) {
		const std::chrono::steady_clock::time_point kNow = std::chrono::steady_clock::now();
		if (kExpiry <= kNow) {
			return false;
		}

		const std::chrono::nanoseconds::rep kRemainingNs
		    = std::chrono::duration_cast<std::chrono::nanoseconds>(kExpiry - kNow).count();
		const DWORD kTimeoutMs = timeoutMilliseconds(kRemainingNs);

		if (WaitOnAddress(&m_state.state, &compare, sizeof(compare), kTimeoutMs) == 0) {
			TRIVIAL_ASSERT(GetLastError() == ERROR_TIMEOUT);
		}
	}

	return true;
}

#elif TRIVIAL_PLATFORM_LINUX

[[nodiscard]] bool Parker::parkFor(std::chrono::nanoseconds lifetime) noexcept {
	const std::chrono::steady_clock::time_point kExpiry = std::chrono::steady_clock::now() + lifetime;

	while (m_state.state.load(std::memory_order_acquire) != 0) {
		const std::chrono::steady_clock::time_point kNow = std::chrono::steady_clock::now();
		if (kExpiry <= kNow) {
			return false;
		}

		const std::chrono::nanoseconds::rep kRemainingNs
		    = std::chrono::duration_cast<std::chrono::nanoseconds>(kExpiry - kNow).count();
		const std::chrono::nanoseconds::rep kRemainingSec = kRemainingNs / TRIVIAL_TIME_NANOSECONDS_PER_SECOND;
		if (kRemainingSec > static_cast<decltype(kRemainingNs)>(std::numeric_limits<std::time_t>::max())) {
			park();
			return true;
		}

		timespec ts{};
		ts.tv_sec = static_cast<std::time_t>(kRemainingSec);
		ts.tv_nsec = static_cast<long>(kRemainingNs % TRIVIAL_TIME_NANOSECONDS_PER_SECOND);

		futexWait(m_state.state, &ts);
	}

	return true;
}

#elif TRIVIAL_PLATFORM_MACOS

[[nodiscard]] bool Parker::parkFor(std::chrono::nanoseconds lifetime) noexcept {
	const std::chrono::steady_clock::time_point kExpiry = std::chrono::steady_clock::now() + lifetime;

	TRIVIAL_VERIFY(pthread_mutex_lock(&m_state.mutex) == 0);

	while (!m_state.notified) {
		const std::chrono::steady_clock::time_point kNow = std::chrono::steady_clock::now();
		if (kExpiry <= kNow) {
			TRIVIAL_VERIFY(pthread_mutex_unlock(&m_state.mutex) == 0);
			return false;
		}

		const std::chrono::nanoseconds::rep kRemainingNs
		    = std::chrono::duration_cast<std::chrono::nanoseconds>(kExpiry - kNow).count();
		const std::chrono::nanoseconds::rep kRemainingSec = kRemainingNs / TRIVIAL_TIME_NANOSECONDS_PER_SECOND;

		timeval wallNow{};
		gettimeofday(&wallNow, nullptr);

		if (kRemainingSec
		    > static_cast<decltype(kRemainingNs)>(std::numeric_limits<std::time_t>::max()) - wallNow.tv_sec) {
			TRIVIAL_VERIFY(pthread_cond_wait(&m_state.condvar, &m_state.mutex) == 0);
			continue;
		}

		long nsec = (static_cast<long>(wallNow.tv_usec) * TRIVIAL_TIME_NANOSECONDS_PER_MICROSECOND)
		            + static_cast<long>(kRemainingNs % TRIVIAL_TIME_NANOSECONDS_PER_SECOND);
		std::time_t sec = wallNow.tv_sec + static_cast<std::time_t>(kRemainingSec);
		if (nsec >= TRIVIAL_TIME_NANOSECONDS_PER_SECOND) {
			nsec -= TRIVIAL_TIME_NANOSECONDS_PER_SECOND;
			sec += 1;
		}

		timespec ts{};
		ts.tv_sec = sec;
		ts.tv_nsec = nsec;

		const int kWaitResult = pthread_cond_timedwait(&m_state.condvar, &m_state.mutex, &ts);
		// NOLINTNEXTLINE(readability-simplify-boolean-expr)
		TRIVIAL_ASSERT(kWaitResult == 0 || kWaitResult == ETIMEDOUT || (ts.tv_sec < 0 && kWaitResult == EINVAL));
		(void)kWaitResult;
	}

	TRIVIAL_VERIFY(pthread_mutex_unlock(&m_state.mutex) == 0);

	return true;
}

#endif // Platform-specific Parker::parkFor

#if TRIVIAL_PLATFORM_WINDOWS || TRIVIAL_PLATFORM_LINUX

[[nodiscard]] UnparkHandle Parker::beginUnpark() noexcept {
	m_state.state.store(0, std::memory_order_release);
	return UnparkHandle(&m_state);
}

#elif TRIVIAL_PLATFORM_MACOS

[[nodiscard]] UnparkHandle Parker::beginUnpark() noexcept {
	TRIVIAL_VERIFY(pthread_mutex_lock(&m_state.mutex) == 0);

	return UnparkHandle(&m_state);
}

#endif // Platform-specific Parker::beginUnpark

#if TRIVIAL_PLATFORM_WINDOWS || TRIVIAL_PLATFORM_LINUX

[[nodiscard]] bool Parker::timedOut() noexcept {
	return m_state.state.load(std::memory_order_relaxed) != 0;
}

#elif TRIVIAL_PLATFORM_MACOS

[[nodiscard]] bool Parker::timedOut() noexcept {
	TRIVIAL_VERIFY(pthread_mutex_lock(&m_state.mutex) == 0);

	const bool kStillWaiting = !m_state.notified;

	TRIVIAL_VERIFY(pthread_mutex_unlock(&m_state.mutex) == 0);

	return kStillWaiting;
}

#endif // Platform-specific Parker::timedOut

} // namespace trivial::sync

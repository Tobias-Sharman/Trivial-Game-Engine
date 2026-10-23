#include "core/sync/parker.h"

#include <atomic>
#include <cerrno>
#include <cstdint>
#include <ctime>
#include <limits>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <trivial/core/assert.h>
#include <trivial/core/time/duration.h>
#include <trivial/core/time/instant.h>
#include <trivial/core/time/time.h>
#include <trivial/core/time/time_constants.h>

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

namespace trivial::sync {

void Parker::wake() noexcept {
	futexWake(m_state);
}

void Parker::park() noexcept {
	while (m_state.load(std::memory_order_acquire) != 0) {
		futexWait(m_state, nullptr);
	}
}

[[nodiscard]] bool Parker::parkFor(time::Duration timeout) noexcept {
	const time::Instant kExpiry = time::now().clampedAdd(timeout);

	while (m_state.load(std::memory_order_acquire) != 0) {
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

		futexWait(m_state, &kTimeout);
	}

	return true;
}

} // namespace trivial::sync

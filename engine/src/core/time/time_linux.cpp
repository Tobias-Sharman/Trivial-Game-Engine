#include <trivial/core/time/time.h>

#include <cerrno>
#include <cstdint>
#include <ctime>
#include <time.h> // NOLINT(modernize-deprecated-headers) -> Not guaranteed to be in ctime by the standard

#include <trivial/core/assert.h>
#include <trivial/core/time/instant.h>
#include <trivial/core/time/time_constants.h>

namespace trivial::time {

std::uint64_t nowTicks() noexcept {
	timespec now{};
	TRIVIAL_VERIFY(clock_gettime(CLOCK_MONOTONIC, &now) == 0);

	return (static_cast<std::uint64_t>(now.tv_sec) * TRIVIAL_TIME_NANOSECONDS_PER_SECOND)
	       + static_cast<std::uint64_t>(now.tv_nsec);
}

void sleepUntil(Instant deadline) noexcept {
	if (deadline.count() <= 0) {
		return;
	}

	const timespec kDeadline{
	    .tv_sec = static_cast<std::time_t>(deadline.count() / TRIVIAL_TIME_NANOSECONDS_PER_SECOND),
	    .tv_nsec = static_cast<long>(deadline.count() % TRIVIAL_TIME_NANOSECONDS_PER_SECOND),
	};

	for (;;) {
		const int kResult = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &kDeadline, nullptr);
		if (kResult != EINTR) {
			TRIVIAL_ASSERT(kResult == 0);
			return;
		}
	}
}

} // namespace trivial::time

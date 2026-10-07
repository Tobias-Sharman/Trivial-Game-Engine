#include <trivial/core/time/time.h>

#include <cstdint>

#include <trivial/core/assert.h>
#include <trivial/core/platform.h>
#include <trivial/core/time/instant.h>

#if TRIVIAL_PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX

#include <windows.h> // IWYU pragma: keep

#include <trivial/core/time/duration.h>

#elif TRIVIAL_PLATFORM_MACOS
#include <limits>
#include <mach/kern_return.h>
#include <mach/mach_time.h>

#elif TRIVIAL_PLATFORM_LINUX
#include <cerrno>
#include <ctime>
#include <time.h> // NOLINT(modernize-deprecated-headers) -> Not guaranteed to be in ctime by the standard

#include <trivial/core/time/time_constants.h>

#endif // Platform check

namespace {

#if TRIVIAL_PLATFORM_WINDOWS

void sleepRelative(trivial::time::Duration duration) noexcept {
	const std::int64_t kNanoseconds = duration.count();
	TRIVIAL_ASSUME(kNanoseconds > 0);

	constexpr std::int64_t kNanosecondsPerDueTimeUnit = 100;

	const HANDLE kTimer // NOLINT(misc-misplaced-const)
	    = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
	TRIVIAL_VERIFY(kTimer != nullptr);

	const LARGE_INTEGER kDueTime{.QuadPart = -(((kNanoseconds - 1) / kNanosecondsPerDueTimeUnit) + 1)};

	TRIVIAL_VERIFY(SetWaitableTimer(kTimer, &kDueTime, 0, nullptr, nullptr, FALSE) != 0);
	TRIVIAL_VERIFY(WaitForSingleObject(kTimer, INFINITE) == WAIT_OBJECT_0);
	TRIVIAL_VERIFY(CloseHandle(kTimer) != 0);
}

#elif TRIVIAL_PLATFORM_MACOS

constexpr std::uint64_t instantToTicks(trivial::time::Instant instant) noexcept {
	const std::int64_t kCount = instant.count();
	TRIVIAL_ASSUME(kCount > 0);

	const std::uint64_t kNanoseconds = static_cast<std::uint64_t>(kCount);
	const std::uint64_t kNumerator = trivial::time::detail::g_detectedCapabilities.timebaseNumerator;
	const std::uint64_t kDenominator = trivial::time::detail::g_detectedCapabilities.timebaseDenominator;
	TRIVIAL_ASSUME(kNumerator != 0);
	TRIVIAL_ASSUME(kDenominator != 0);

	const std::uint64_t kWhole = kNanoseconds / kNumerator;
	const std::uint64_t kPartial = (((kNanoseconds % kNumerator) * kDenominator) + kNumerator - 1) / kNumerator;

	if (kWhole > (std::numeric_limits<std::uint64_t>::max() - kPartial) / kDenominator) {
		return std::numeric_limits<std::uint64_t>::max();
	}

	return (kWhole * kDenominator) + kPartial;
}

#endif // Platform check

} // namespace

namespace trivial::time {

std::uint64_t nowTicks() noexcept {
#if TRIVIAL_PLATFORM_WINDOWS
	LARGE_INTEGER counter{};
	TRIVIAL_VERIFY(QueryPerformanceCounter(&counter) != 0);

	return static_cast<std::uint64_t>(counter.QuadPart);

#elif TRIVIAL_PLATFORM_MACOS
	return mach_absolute_time();

#elif TRIVIAL_PLATFORM_LINUX
	timespec now{};
	TRIVIAL_VERIFY(clock_gettime(CLOCK_MONOTONIC, &now) == 0);

	return (static_cast<std::uint64_t>(now.tv_sec) * TRIVIAL_TIME_NANOSECONDS_PER_SECOND)
	       + static_cast<std::uint64_t>(now.tv_nsec);

#endif // Platform check
}

void sleepUntil(Instant deadline) noexcept {
#if TRIVIAL_PLATFORM_WINDOWS
	for (;;) {
		const Instant kNow = now();
		if (deadline <= kNow) {
			return;
		}

		sleepRelative(deadline - kNow);
	}

#elif TRIVIAL_PLATFORM_MACOS
	if (deadline.count() <= 0) {
		return;
	}

	const std::uint64_t kDeadlineTicks = instantToTicks(deadline);

	for (;;) {
		const kern_return_t kResult = mach_wait_until(kDeadlineTicks);
		if (kResult != KERN_ABORTED) {
			TRIVIAL_ASSERT(kResult == KERN_SUCCESS);
			return;
		}
	}

#elif TRIVIAL_PLATFORM_LINUX
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

#endif // Platform check
}

} // namespace trivial::time

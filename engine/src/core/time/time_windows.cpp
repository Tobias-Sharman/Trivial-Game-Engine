#include <trivial/core/time/time.h>

#include <cstdint>

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

namespace {

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

} // namespace

namespace trivial::time {

std::uint64_t nowTicks() noexcept {
	LARGE_INTEGER counter{};
	TRIVIAL_VERIFY(QueryPerformanceCounter(&counter) != 0);

	return static_cast<std::uint64_t>(counter.QuadPart);
}

void sleepUntil(Instant deadline) noexcept {
	for (;;) {
		const Instant kNow = now();
		if (deadline <= kNow) {
			return;
		}

		sleepRelative(deadline - kNow);
	}
}

} // namespace trivial::time

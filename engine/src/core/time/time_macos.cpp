#include <trivial/core/time/time.h>

#include <cstdint>
#include <limits>
#include <mach/kern_return.h>
#include <mach/mach_time.h>

#include <trivial/core/assert.h>
#include <trivial/core/time/instant.h>

namespace {

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

} // namespace

namespace trivial::time {

std::uint64_t nowTicks() noexcept {
	return mach_absolute_time();
}

void sleepUntil(Instant deadline) noexcept {
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
}

} // namespace trivial::time

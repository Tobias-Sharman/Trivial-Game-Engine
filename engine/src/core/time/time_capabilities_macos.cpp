#include "core/time/time_capabilities.h"

#include <cstdint>
#include <mach/kern_return.h>
#include <mach/mach_time.h>

#include <trivial/core/assert.h>
#include <trivial/core/compiler.h>
#include <trivial/core/time/time.h>
#include <trivial/core/time/time_constants.h>

namespace {

struct Timebase {
	std::uint32_t numerator = 0;
	std::uint32_t denominator = 0;
};

[[nodiscard]] TRIVIAL_FORCE_INLINE Timebase probeTimebase() noexcept {
	mach_timebase_info_data_t info{};
	TRIVIAL_VERIFY(mach_timebase_info(&info) == KERN_SUCCESS);

	return Timebase{.numerator = info.numer, .denominator = info.denom};
}

} // namespace

namespace trivial::time {

void initCapabilities() noexcept {
	const Timebase kTimebase = probeTimebase();
	TRIVIAL_ASSERT(kTimebase.numerator != 0);
	TRIVIAL_ASSERT(kTimebase.denominator != 0);

	const std::uint64_t kScaledDenominator = std::uint64_t{TRIVIAL_TIME_NANOSECONDS_PER_SECOND} * kTimebase.denominator;

	TRIVIAL_ASSERT(kScaledDenominator % kTimebase.numerator == 0);
	detail::g_detectedCapabilities.ticksPerSecond = kScaledDenominator / kTimebase.numerator;
	detail::g_detectedCapabilities.timebaseNumerator = kTimebase.numerator;
	detail::g_detectedCapabilities.timebaseDenominator = kTimebase.denominator;
}

} // namespace trivial::time

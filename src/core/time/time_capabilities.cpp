#include "core/time/time_capabilities.h"

#include <trivial/core/platform.h>

#define TRIVIAL_TIME_CAPABILITIES_IMPLEMENTATION
#include <trivial/core/time/time.h>

#if TRIVIAL_TIME_HAS_DETECTED_CAPABILITIES
#include <cstdint>

#include <trivial/core/assert.h>
#endif // TRIVIAL_TIME_HAS_DETECTED_CAPABILITIES

#if TRIVIAL_PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX

#include <windows.h> // IWYU pragma: keep

#elif TRIVIAL_PLATFORM_MACOS
#include <mach/kern_return.h>
#include <mach/mach_time.h>

#include <trivial/core/time/time_constants.h>

#endif // Platform check

#if TRIVIAL_TIME_HAS_DETECTED_CAPABILITIES
namespace {

#if TRIVIAL_PLATFORM_WINDOWS
[[nodiscard]] std::uint64_t probePerformanceFrequency() noexcept {
	LARGE_INTEGER frequency{};
	TRIVIAL_VERIFY(QueryPerformanceFrequency(&frequency) != 0);

	return static_cast<std::uint64_t>(frequency.QuadPart);
}

#elif TRIVIAL_PLATFORM_MACOS
struct Timebase {
	std::uint32_t numerator = 0;
	std::uint32_t denominator = 0;
};

[[nodiscard]] Timebase probeTimebase() noexcept {
	mach_timebase_info_data_t info{};
	TRIVIAL_VERIFY(mach_timebase_info(&info) == KERN_SUCCESS);

	return Timebase{.numerator = info.numer, .denominator = info.denom};
}

#endif // Platform check

} // namespace
#endif // TRIVIAL_TIME_HAS_DETECTED_CAPABILITIES

namespace trivial::time {

void initCapabilities() noexcept {
#if TRIVIAL_PLATFORM_WINDOWS
	detail::g_detectedCapabilities = detail::DetectedCapabilities{.ticksPerSecond = probePerformanceFrequency()};

#elif TRIVIAL_PLATFORM_MACOS
	const Timebase kTimebase = probeTimebase();
	TRIVIAL_ASSERT(kTimebase.numerator != 0);
	TRIVIAL_ASSERT(kTimebase.denominator != 0);

	const std::uint64_t kScaledDenominator = std::uint64_t{TRIVIAL_TIME_NANOSECONDS_PER_SECOND} * kTimebase.denominator;

	TRIVIAL_ASSERT(kScaledDenominator % kTimebase.numerator == 0);
	detail::g_detectedCapabilities = detail::DetectedCapabilities{
	    .ticksPerSecond = kScaledDenominator / kTimebase.numerator,
	    .timebaseNumerator = kTimebase.numerator,
	    .timebaseDenominator = kTimebase.denominator,
	};

#endif // Platform check
}

} // namespace trivial::time

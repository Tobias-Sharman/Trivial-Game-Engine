#ifndef TRIVIAL_CORE_TIME_TIME_H
#define TRIVIAL_CORE_TIME_TIME_H

#include <cstdint>

#include <trivial/core/assert.h>
#include <trivial/core/compiler.h>
#include <trivial/core/platform.h>
#include <trivial/core/time/duration.h>
#include <trivial/core/time/instant.h>
#include <trivial/core/time/time_constants.h>

#define TRIVIAL_TIME_HAS_DETECTED_CAPABILITIES (TRIVIAL_PLATFORM_WINDOWS || TRIVIAL_PLATFORM_MACOS)

#define TRIVIAL_TIME_FREQUENCY_10_MHZ 10'000'000U
#define TRIVIAL_TIME_FREQUENCY_24_MHZ 24'000'000U
#define TRIVIAL_TIME_FREQUENCY_24_MHZ_NUMERATOR 125U
#define TRIVIAL_TIME_FREQUENCY_24_MHZ_DENOMINATOR 3U

#if !TRIVIAL_TIME_HAS_DETECTED_CAPABILITIES
#include <type_traits>

#endif // !TRIVIAL_TIME_HAS_DETECTED_CAPABILITIES

namespace trivial::time {

namespace detail {

struct DetectedCapabilities {
#if TRIVIAL_PLATFORM_WINDOWS || TRIVIAL_PLATFORM_MACOS
	std::uint64_t ticksPerSecond = 0;
#endif // TRIVIAL_PLATFORM_WINDOWS || TRIVIAL_PLATFORM_MACOS

#if TRIVIAL_PLATFORM_MACOS
	std::uint32_t timebaseNumerator = 0;
	std::uint32_t timebaseDenominator = 0;
#endif // TRIVIAL_PLATFORM_MACOS
};

#if TRIVIAL_TIME_HAS_DETECTED_CAPABILITIES
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
inline constinit DetectedCapabilities g_detectedCapabilities;

#else
static_assert(std::is_empty_v<DetectedCapabilities>,
              "A detected capability was added without updating TRIVIAL_TIME_HAS_DETECTED_CAPABILITIES");

#endif // TRIVIAL_TIME_HAS_DETECTED_CAPABILITIES

} // namespace detail

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr std::uint64_t scaleTicks(std::uint64_t ticks,
                                                                      std::uint64_t numerator,
                                                                      std::uint64_t denominator) noexcept {
	TRIVIAL_ASSUME(denominator != 0);
	return ((ticks / denominator) * numerator) + (((ticks % denominator) * numerator) / denominator);
}

static_assert(std::uint64_t{TRIVIAL_TIME_NANOSECONDS_PER_SECOND} % TRIVIAL_TIME_FREQUENCY_10_MHZ == 0);
static_assert(std::uint64_t{TRIVIAL_TIME_NANOSECONDS_PER_SECOND} * TRIVIAL_TIME_FREQUENCY_24_MHZ_DENOMINATOR
              == std::uint64_t{TRIVIAL_TIME_FREQUENCY_24_MHZ} * TRIVIAL_TIME_FREQUENCY_24_MHZ_NUMERATOR);

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr std::uint64_t ticksPerSecond() noexcept {
#if TRIVIAL_TIME_HAS_DETECTED_CAPABILITIES
	TRIVIAL_ASSERT(detail::g_detectedCapabilities.ticksPerSecond != 0);
	return detail::g_detectedCapabilities.ticksPerSecond;
#else
	return TRIVIAL_TIME_NANOSECONDS_PER_SECOND;
#endif // TRIVIAL_TIME_HAS_DETECTED_CAPABILITIES
}

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr Duration ticksToDuration(std::uint64_t ticks) noexcept {
#if TRIVIAL_PLATFORM_WINDOWS
	const std::uint64_t kTicksPerSecond = ticksPerSecond();

	if constexpr (TRIVIAL_ARCH_ARM64) {
		if (kTicksPerSecond == TRIVIAL_TIME_FREQUENCY_24_MHZ) [[likely]] {
			return Duration::fromNanoseconds(static_cast<std::int64_t>(
			    scaleTicks(ticks, TRIVIAL_TIME_FREQUENCY_24_MHZ_NUMERATOR, TRIVIAL_TIME_FREQUENCY_24_MHZ_DENOMINATOR)));
		}

		if (kTicksPerSecond == TRIVIAL_TIME_FREQUENCY_10_MHZ) {
			return Duration::fromNanoseconds(static_cast<std::int64_t>(
			    ticks * (TRIVIAL_TIME_NANOSECONDS_PER_SECOND / TRIVIAL_TIME_FREQUENCY_10_MHZ)));
		}
	} else {
		if (kTicksPerSecond == TRIVIAL_TIME_FREQUENCY_10_MHZ) [[likely]] {
			return Duration::fromNanoseconds(static_cast<std::int64_t>(
			    ticks * (TRIVIAL_TIME_NANOSECONDS_PER_SECOND / TRIVIAL_TIME_FREQUENCY_10_MHZ)));
		}

		if (kTicksPerSecond == TRIVIAL_TIME_FREQUENCY_24_MHZ) {
			return Duration::fromNanoseconds(static_cast<std::int64_t>(
			    scaleTicks(ticks, TRIVIAL_TIME_FREQUENCY_24_MHZ_NUMERATOR, TRIVIAL_TIME_FREQUENCY_24_MHZ_DENOMINATOR)));
		}
	}

	return Duration::fromNanoseconds(
	    static_cast<std::int64_t>(scaleTicks(ticks, TRIVIAL_TIME_NANOSECONDS_PER_SECOND, kTicksPerSecond)));

#elif TRIVIAL_PLATFORM_MACOS
	const std::uint32_t kNumerator = detail::g_detectedCapabilities.timebaseNumerator;
	const std::uint32_t kDenominator = detail::g_detectedCapabilities.timebaseDenominator;

	if constexpr (TRIVIAL_ARCH_ARM64) {
		if (kNumerator == TRIVIAL_TIME_FREQUENCY_24_MHZ_NUMERATOR
		    && kDenominator == TRIVIAL_TIME_FREQUENCY_24_MHZ_DENOMINATOR) [[likely]] {
			return Duration::fromNanoseconds(static_cast<std::int64_t>(
			    scaleTicks(ticks, TRIVIAL_TIME_FREQUENCY_24_MHZ_NUMERATOR, TRIVIAL_TIME_FREQUENCY_24_MHZ_DENOMINATOR)));
		}
	} else {
		if (kNumerator == 1 && kDenominator == 1) [[likely]] {
			return Duration::fromNanoseconds(static_cast<std::int64_t>(ticks));
		}
	}

	return Duration::fromNanoseconds(static_cast<std::int64_t>(scaleTicks(ticks, kNumerator, kDenominator)));

#else
	return Duration::fromNanoseconds(static_cast<std::int64_t>(ticks));

#endif // Platform check
}

[[nodiscard]] std::uint64_t nowTicks() noexcept;

[[nodiscard]] TRIVIAL_FORCE_INLINE Instant now() noexcept {
	return Instant::fromNanoseconds(ticksToDuration(nowTicks()).count());
}

// Pure OS sleeps, which can wake up to ~0.5 ms late (less on macOS/Linux).
// Frame pacing should use a separate precise variant rather than changing
// these, which would sleep then spin
void sleepUntil(Instant deadline) noexcept;
TRIVIAL_FORCE_INLINE void sleepFor(Duration duration) noexcept {
	sleepUntil(now().clampedAdd(duration));
}

} // namespace trivial::time

#undef TRIVIAL_TIME_FREQUENCY_10_MHZ
#undef TRIVIAL_TIME_FREQUENCY_24_MHZ
#undef TRIVIAL_TIME_FREQUENCY_24_MHZ_NUMERATOR
#undef TRIVIAL_TIME_FREQUENCY_24_MHZ_DENOMINATOR

#ifndef TRIVIAL_TIME_CAPABILITIES_IMPLEMENTATION
#undef TRIVIAL_TIME_HAS_DETECTED_CAPABILITIES
#endif // TRIVIAL_TIME_CAPABILITIES_IMPLEMENTATION

#endif // TRIVIAL_CORE_TIME_TIME_H

#ifndef TRIVIAL_WORLD_WORLD_TIME_H
#define TRIVIAL_WORLD_WORLD_TIME_H

#include <cmath>
#include <cstdint>
#include <limits>

#include <trivial/core/assert.h>
#include <trivial/core/time/duration.h>

namespace trivial::world {

// NOTE: game seconds are when not paused
class WorldTime {
public:
	void tick(time::Duration realDelta) noexcept {
		const bool kNonNegative = realDelta >= time::Duration{};
		TRIVIAL_ASSUME(kNonNegative);

		m_realDelta = realDelta;
		m_realElapsed += m_realDelta;

		m_unpausedDelta = scaleDelta(m_realDelta, m_timeScale, m_unpausedRemainder);
		m_unpausedElapsed += m_unpausedDelta;

		m_gameDelta = scaleDelta(m_unpausedDelta, m_pauseMultiplier, m_gameRemainder);
		m_gameElapsed += m_gameDelta;
	}

	constexpr void reset() noexcept {
		m_realDelta = time::Duration{};
		m_realElapsed = time::Duration{};

		m_unpausedDelta = time::Duration{};
		m_unpausedElapsed = time::Duration{};
		m_unpausedRemainder = 0.0;

		m_gameDelta = time::Duration{};
		m_gameElapsed = time::Duration{};
		m_gameRemainder = 0.0;
	}

	[[nodiscard]] constexpr double pauseMultiplier() const noexcept { return m_pauseMultiplier; }
	constexpr void setPauseMultiplier(double pauseMultiplier) noexcept { m_pauseMultiplier = pauseMultiplier; }

	[[nodiscard]] constexpr double timeScale() const noexcept { return m_timeScale; }
	constexpr void setTimeScale(double scale) noexcept { m_timeScale = scale; }

	[[nodiscard]] constexpr time::Duration realDelta() const noexcept { return m_realDelta; }
	[[nodiscard]] constexpr double realDeltaSeconds() const noexcept { return m_realDelta.toSecondsDouble(); }
	[[nodiscard]] constexpr time::Duration realElapsed() const noexcept { return m_realElapsed; }

	[[nodiscard]] constexpr time::Duration unpausedDelta() const noexcept { return m_unpausedDelta; }
	[[nodiscard]] constexpr double unpausedDeltaSeconds() const noexcept { return m_unpausedDelta.toSecondsDouble(); }
	[[nodiscard]] constexpr time::Duration unpausedElapsed() const noexcept { return m_unpausedElapsed; }

	[[nodiscard]] constexpr time::Duration gameDelta() const noexcept { return m_gameDelta; }
	[[nodiscard]] constexpr double gameDeltaSeconds() const noexcept { return m_gameDelta.toSecondsDouble(); }
	[[nodiscard]] constexpr time::Duration gameElapsed() const noexcept { return m_gameElapsed; }

private:
	[[nodiscard]] static time::Duration scaleDelta(time::Duration delta, double scale, double& outRemainder) noexcept {
		constexpr double kInt64Bound = -static_cast<double>(std::numeric_limits<std::int64_t>::min());

		const double kScaled = (static_cast<double>(delta.count()) * scale) + outRemainder;
		const double kWhole = std::floor(kScaled);
		TRIVIAL_ASSUME(kWhole >= -kInt64Bound);
		TRIVIAL_ASSUME(kWhole < kInt64Bound);

		outRemainder = kScaled - kWhole;

		return time::Duration::fromNanoseconds(static_cast<std::int64_t>(kWhole));
	}

	time::Duration m_realDelta;
	time::Duration m_realElapsed;

	time::Duration m_unpausedDelta;
	time::Duration m_unpausedElapsed;
	double m_unpausedRemainder = 0.0;

	time::Duration m_gameDelta;
	time::Duration m_gameElapsed;
	double m_gameRemainder = 0.0;

	double m_timeScale = 1.0;
	double m_pauseMultiplier = 1.0;
};

} // namespace trivial::world

#endif // TRIVIAL_WORLD_WORLD_TIME_H

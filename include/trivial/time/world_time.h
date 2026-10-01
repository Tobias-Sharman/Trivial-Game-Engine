#ifndef TRIVIAL_TIME_WORLD_TIME_H
#define TRIVIAL_TIME_WORLD_TIME_H

#include <algorithm>

#include <trivial/time/engine_time.h>

// NOTE: game seconds are when not paused
class WorldTime {
public:
	void tick(const EngineTime& engineTime) noexcept {
		m_realDeltaSeconds = engineTime.deltaSeconds();
		m_realElapsedSeconds += m_realDeltaSeconds;

		m_unpausedDeltaSeconds = m_realDeltaSeconds * m_timeScale;
		m_unpausedElapsedSeconds += m_unpausedDeltaSeconds;

		m_gameDeltaSeconds = m_unpausedDeltaSeconds * m_pauseMultiplier;
		m_gameElapsedSeconds += m_gameDeltaSeconds;
	}

	constexpr void reset() noexcept {
		m_realDeltaSeconds = 0.0;
		m_realElapsedSeconds = 0.0;

		m_unpausedDeltaSeconds = 0.0;
		m_unpausedElapsedSeconds = 0.0;

		m_gameDeltaSeconds = 0.0;
		m_gameElapsedSeconds = 0.0;
	}

	[[nodiscard]] constexpr double pauseMultiplier() const noexcept { return m_pauseMultiplier; }
	constexpr void setPauseMultiplier(double pauseMultiplier) noexcept { m_pauseMultiplier = pauseMultiplier; }

	[[nodiscard]] constexpr double timeScale() const noexcept { return m_timeScale; }
	constexpr void setTimeScale(double scale) noexcept { m_timeScale = std::max(scale, 0.0); }

	[[nodiscard]] constexpr double realDeltaSeconds() const noexcept { return m_realDeltaSeconds; }
	[[nodiscard]] constexpr double realElapsedSeconds() const noexcept { return m_realElapsedSeconds; }

	[[nodiscard]] constexpr double unpausedDeltaSeconds() const noexcept { return m_unpausedDeltaSeconds; }
	[[nodiscard]] constexpr double unpausedElapsedSeconds() const noexcept { return m_unpausedElapsedSeconds; }

	[[nodiscard]] constexpr double gameDeltaSeconds() const noexcept { return m_gameDeltaSeconds; }
	[[nodiscard]] constexpr double gameElapsedSeconds() const noexcept { return m_gameElapsedSeconds; }

private:
	double m_realDeltaSeconds = 0.0;
	double m_realElapsedSeconds = 0.0;

	double m_unpausedDeltaSeconds = 0.0;
	double m_unpausedElapsedSeconds = 0.0;

	double m_gameDeltaSeconds = 0.0;
	double m_gameElapsedSeconds = 0.0;

	double m_timeScale = 1.0;
	double m_pauseMultiplier = 1.0;
};

#endif // TRIVIAL_TIME_WORLD_TIME_H

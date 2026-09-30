#ifndef TRIVIAL_TIME_ENGINE_TIME_H
#define TRIVIAL_TIME_ENGINE_TIME_H

#include <algorithm>
#include <chrono>

#include <trivial/time/time_config.h>

class EngineTime {
	using Clock = std::chrono::steady_clock;

public:
	EngineTime() { reset(); }

	void reset() {
		m_start = Clock::now();
		m_previous = m_start;

		m_rawDeltaSeconds = 0.0;
		m_deltaSeconds = 0.0;
	}

	void tick() {
		const Clock::time_point kNow = Clock::now();

		m_rawDeltaSeconds = std::chrono::duration<double>(kNow - m_previous).count();

		m_previous = kNow;

		m_deltaSeconds = std::clamp(m_rawDeltaSeconds, 0.0, TRIVIAL_TIME_MAX_DELTA_SECONDS);
	}

	[[nodiscard]] double deltaSeconds() const noexcept { return m_deltaSeconds; }

	[[nodiscard]] double rawDeltaSeconds() const noexcept { return m_rawDeltaSeconds; }

private:
	Clock::time_point m_start;
	Clock::time_point m_previous;

	double m_rawDeltaSeconds = 0.0; // NOTE: Could screen this for release build
	double m_deltaSeconds = 0.0;

	// NOTE: Maybe add a frame counter and elapsed seconds when making a proper debug overlay
};

#endif // TRIVIAL_TIME_ENGINE_TIME_H

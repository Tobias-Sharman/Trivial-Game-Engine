#ifndef TRIVIAL_TIME_ENGINE_TIME_H
#define TRIVIAL_TIME_ENGINE_TIME_H

#include <algorithm>
#include <chrono>

class EngineTime {
	using Clock = std::chrono::steady_clock;

	constexpr static double kBaseMaxDeltaSeconds = 0.25;

public:
	explicit EngineTime(double maximumDeltaSeconds = kBaseMaxDeltaSeconds)
	    : m_maximumDeltaSeconds(maximumDeltaSeconds) {
		reset();
	}

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

		m_deltaSeconds = std::clamp(m_rawDeltaSeconds, 0.0, m_maximumDeltaSeconds);
	}

	[[nodiscard]] double deltaSeconds() const noexcept { return m_deltaSeconds; }

	[[nodiscard]] double rawDeltaSeconds() const noexcept { return m_rawDeltaSeconds; }

	[[nodiscard]] double maximumDeltaSeconds() const noexcept { return m_maximumDeltaSeconds; }

	void setMaximumDeltaSeconds(double value) noexcept { m_maximumDeltaSeconds = std::max(value, 0.0); }

private:
	Clock::time_point m_start;
	Clock::time_point m_previous;

	double m_rawDeltaSeconds = 0.0; // NOTE: Could screen this for release build
	double m_deltaSeconds = 0.0;

	// NOTE: Maybe add a frame counter and elapsed seconds when making a proper debug overlay

	double m_maximumDeltaSeconds = kBaseMaxDeltaSeconds;
};

#endif // TRIVIAL_TIME_ENGINE_TIME_H

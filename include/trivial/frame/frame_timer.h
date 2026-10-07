#ifndef TRIVIAL_FRAME_FRAME_TIMER_H
#define TRIVIAL_FRAME_FRAME_TIMER_H

#include <algorithm>

#include <trivial/core/time/duration.h>
#include <trivial/core/time/instant.h>
#include <trivial/core/time/time.h>
#include <trivial/frame/frame_config.h>

namespace trivial {

class FrameTimer {
public:
	FrameTimer() noexcept = default;

	void reset() noexcept {
		m_start = time::now();
		m_previous = m_start;

		m_rawDelta = time::Duration{};
		m_delta = time::Duration{};
	}

	void tick() noexcept {
		const time::Instant kNow = time::now();

		m_rawDelta = kNow - m_previous;

		m_previous = kNow;

		constexpr time::Duration kMaxDelta = time::milliseconds(TRIVIAL_FRAME_MAX_DELTA_MILLISECONDS);
		m_delta = std::clamp(m_rawDelta, time::Duration{}, kMaxDelta);
	}

	[[nodiscard]] time::Duration delta() const noexcept { return m_delta; }
	[[nodiscard]] double deltaSeconds() const noexcept { return m_delta.toSecondsDouble(); }

	[[nodiscard]] time::Duration rawDelta() const noexcept { return m_rawDelta; }
	[[nodiscard]] double rawDeltaSeconds() const noexcept { return m_rawDelta.toSecondsDouble(); }

	[[nodiscard]] time::Duration elapsed() const noexcept { return m_previous - m_start; }
	[[nodiscard]] double elapsedSeconds() const noexcept { return elapsed().toSecondsDouble(); }

private:
	time::Instant m_start;
	time::Instant m_previous;

	time::Duration m_rawDelta; // NOTE: Could screen this for release build
	time::Duration m_delta;
};

} // namespace trivial

#endif // TRIVIAL_FRAME_FRAME_TIMER_H

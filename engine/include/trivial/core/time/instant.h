#ifndef TRIVIAL_CORE_TIME_INSTANT_H
#define TRIVIAL_CORE_TIME_INSTANT_H

#include <cstdint>
#include <limits>
#include <type_traits>

#include <trivial/core/time/duration.h>

namespace trivial::time {

struct Instant {
	[[nodiscard]] static constexpr Instant fromNanoseconds(std::int64_t nanoseconds) noexcept {
		Instant result;
		result.m_nanoseconds = nanoseconds;
		return result;
	}

	[[nodiscard]] constexpr std::int64_t count() const noexcept { return m_nanoseconds; }

	[[nodiscard]] constexpr Instant clampedAdd(Duration duration) const noexcept {
		const std::int64_t kNanoseconds = duration.count();

		if (kNanoseconds > 0 && m_nanoseconds > std::numeric_limits<std::int64_t>::max() - kNanoseconds) {
			return fromNanoseconds(std::numeric_limits<std::int64_t>::max());
		}

		if (kNanoseconds < 0 && m_nanoseconds < std::numeric_limits<std::int64_t>::min() - kNanoseconds) {
			return fromNanoseconds(std::numeric_limits<std::int64_t>::min());
		}

		return fromNanoseconds(m_nanoseconds + kNanoseconds);
	}

	[[nodiscard]] constexpr Duration operator-(Instant rhs) const noexcept {
		return Duration::fromNanoseconds(m_nanoseconds - rhs.m_nanoseconds);
	}

	[[nodiscard]] constexpr Instant operator+(Duration rhs) const noexcept {
		Instant result = *this;
		result.m_nanoseconds += rhs.count();
		return result;
	}

	[[nodiscard]] constexpr Instant operator-(Duration rhs) const noexcept {
		Instant result = *this;
		result.m_nanoseconds -= rhs.count();
		return result;
	}

	constexpr Instant& operator+=(Duration rhs) noexcept {
		m_nanoseconds += rhs.count();
		return *this;
	}

	constexpr Instant& operator-=(Duration rhs) noexcept {
		m_nanoseconds -= rhs.count();
		return *this;
	}

	[[nodiscard]] constexpr bool operator==(const Instant&) const noexcept = default;

	[[nodiscard]] constexpr bool operator<(Instant rhs) const noexcept { return m_nanoseconds < rhs.m_nanoseconds; }
	[[nodiscard]] constexpr bool operator<=(Instant rhs) const noexcept { return m_nanoseconds <= rhs.m_nanoseconds; }
	[[nodiscard]] constexpr bool operator>(Instant rhs) const noexcept { return m_nanoseconds > rhs.m_nanoseconds; }
	[[nodiscard]] constexpr bool operator>=(Instant rhs) const noexcept { return m_nanoseconds >= rhs.m_nanoseconds; }

private:
	std::int64_t m_nanoseconds = 0;
};

[[nodiscard]] constexpr Instant operator+(Duration duration, Instant instant) noexcept {
	return instant + duration;
}

static_assert(sizeof(Instant) == sizeof(std::int64_t));
static_assert(alignof(Instant) == alignof(std::int64_t));

static_assert(std::is_trivially_copyable_v<Instant>);
static_assert(std::is_standard_layout_v<Instant>);
static_assert(std::is_nothrow_default_constructible_v<Instant>);

static_assert(!std::is_aggregate_v<Instant>);
static_assert(!std::is_constructible_v<Instant, std::int64_t>);
static_assert(!std::is_convertible_v<std::int64_t, Instant>);
static_assert(!std::is_constructible_v<Instant, Duration>);

} // namespace trivial::time

#endif // TRIVIAL_CORE_TIME_INSTANT_H

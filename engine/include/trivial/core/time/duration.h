#ifndef TRIVIAL_CORE_TIME_DURATION_H
#define TRIVIAL_CORE_TIME_DURATION_H

#include <cstdint>
#include <limits>
#include <type_traits>

#include <trivial/core/assert.h>
#include <trivial/core/compiler.h>
#include <trivial/core/time/time_constants.h>

namespace trivial::time {

struct Duration {
	TRIVIAL_FORCE_INLINE constexpr Duration() noexcept = default;

	[[nodiscard]] TRIVIAL_FORCE_INLINE static constexpr Duration fromNanoseconds(std::int64_t nanoseconds) noexcept {
		return Duration(nanoseconds);
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr std::int64_t count() const noexcept { return m_nanoseconds; }

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr std::int64_t toMicroseconds() const noexcept {
		return m_nanoseconds / TRIVIAL_TIME_NANOSECONDS_PER_MICROSECOND;
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr std::int64_t toMilliseconds() const noexcept {
		return m_nanoseconds / TRIVIAL_TIME_NANOSECONDS_PER_MILLISECOND;
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr std::int64_t toMillisecondsCeil() const noexcept {
		const std::int64_t kWhole = m_nanoseconds / TRIVIAL_TIME_NANOSECONDS_PER_MILLISECOND;
		return m_nanoseconds % TRIVIAL_TIME_NANOSECONDS_PER_MILLISECOND > 0 ? kWhole + 1 : kWhole;
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr std::int64_t toSeconds() const noexcept {
		return m_nanoseconds / TRIVIAL_TIME_NANOSECONDS_PER_SECOND;
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr double toSecondsDouble() const noexcept {
		return static_cast<double>(m_nanoseconds) / static_cast<double>(TRIVIAL_TIME_NANOSECONDS_PER_SECOND);
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr Duration operator+() const noexcept { return *this; }
	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr Duration operator-() const noexcept {
		return Duration(-m_nanoseconds);
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr Duration operator+(Duration rhs) const noexcept {
		return Duration(m_nanoseconds + rhs.m_nanoseconds);
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr Duration operator-(Duration rhs) const noexcept {
		return Duration(m_nanoseconds - rhs.m_nanoseconds);
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr Duration operator*(std::int64_t scalar) const noexcept {
		return Duration(m_nanoseconds * scalar);
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr Duration operator/(std::int64_t scalar) const noexcept {
		constexpr std::int64_t kMin = std::numeric_limits<std::int64_t>::min();
		TRIVIAL_ASSUME(scalar != 0);
		TRIVIAL_ASSUME(!(m_nanoseconds == kMin && scalar == -1)); // NOLINT(readability-simplify-boolean-expr)

		return Duration(m_nanoseconds / scalar);
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr std::int64_t operator/(Duration rhs) const noexcept {
		constexpr std::int64_t kMin = std::numeric_limits<std::int64_t>::min();
		TRIVIAL_ASSUME(rhs.m_nanoseconds != 0);
		// NOLINTNEXTLINE(readability-simplify-boolean-expr)
		TRIVIAL_ASSUME(!(m_nanoseconds == kMin && rhs.m_nanoseconds == -1));

		return m_nanoseconds / rhs.m_nanoseconds;
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr Duration operator%(Duration rhs) const noexcept {
		constexpr std::int64_t kMin = std::numeric_limits<std::int64_t>::min();
		TRIVIAL_ASSUME(rhs.m_nanoseconds != 0);
		// NOLINTNEXTLINE(readability-simplify-boolean-expr)
		TRIVIAL_ASSUME(!(m_nanoseconds == kMin && rhs.m_nanoseconds == -1));

		return Duration(m_nanoseconds % rhs.m_nanoseconds);
	}

	TRIVIAL_FORCE_INLINE constexpr Duration& operator+=(Duration rhs) noexcept {
		m_nanoseconds += rhs.m_nanoseconds;
		return *this;
	}

	TRIVIAL_FORCE_INLINE constexpr Duration& operator-=(Duration rhs) noexcept {
		m_nanoseconds -= rhs.m_nanoseconds;
		return *this;
	}

	TRIVIAL_FORCE_INLINE constexpr Duration& operator*=(std::int64_t scalar) noexcept {
		m_nanoseconds *= scalar;
		return *this;
	}

	TRIVIAL_FORCE_INLINE constexpr Duration& operator/=(std::int64_t scalar) noexcept {
		constexpr std::int64_t kMin = std::numeric_limits<std::int64_t>::min();
		TRIVIAL_ASSUME(scalar != 0);
		TRIVIAL_ASSUME(!(m_nanoseconds == kMin && scalar == -1)); // NOLINT(readability-simplify-boolean-expr)

		m_nanoseconds /= scalar;
		return *this;
	}

	TRIVIAL_FORCE_INLINE constexpr Duration& operator%=(Duration rhs) noexcept {
		constexpr std::int64_t kMin = std::numeric_limits<std::int64_t>::min();
		TRIVIAL_ASSUME(rhs.m_nanoseconds != 0);
		// NOLINTNEXTLINE(readability-simplify-boolean-expr)
		TRIVIAL_ASSUME(!(m_nanoseconds == kMin && rhs.m_nanoseconds == -1));

		m_nanoseconds %= rhs.m_nanoseconds;
		return *this;
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr bool operator==(const Duration&) const noexcept = default;

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr bool operator<(Duration rhs) const noexcept {
		return m_nanoseconds < rhs.m_nanoseconds;
	}
	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr bool operator<=(Duration rhs) const noexcept {
		return m_nanoseconds <= rhs.m_nanoseconds;
	}
	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr bool operator>(Duration rhs) const noexcept {
		return m_nanoseconds > rhs.m_nanoseconds;
	}
	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr bool operator>=(Duration rhs) const noexcept {
		return m_nanoseconds >= rhs.m_nanoseconds;
	}

private:
	TRIVIAL_FORCE_INLINE constexpr explicit Duration(std::int64_t nanoseconds) noexcept
	    : m_nanoseconds(nanoseconds) {}

	std::int64_t m_nanoseconds = 0;
};

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr Duration operator*(std::int64_t scalar, Duration duration) noexcept {
	return duration * scalar;
}

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr Duration nanoseconds(std::int64_t count) noexcept {
	return Duration::fromNanoseconds(count);
}

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr Duration microseconds(std::int64_t count) noexcept {
	return Duration::fromNanoseconds(count * TRIVIAL_TIME_NANOSECONDS_PER_MICROSECOND);
}

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr Duration milliseconds(std::int64_t count) noexcept {
	return Duration::fromNanoseconds(count * TRIVIAL_TIME_NANOSECONDS_PER_MILLISECOND);
}

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr Duration seconds(std::int64_t count) noexcept {
	return Duration::fromNanoseconds(count * TRIVIAL_TIME_NANOSECONDS_PER_SECOND);
}

static_assert(sizeof(Duration) == sizeof(std::int64_t));
static_assert(alignof(Duration) == alignof(std::int64_t));

static_assert(std::is_trivially_copyable_v<Duration>);
static_assert(std::is_standard_layout_v<Duration>);
static_assert(std::is_nothrow_default_constructible_v<Duration>);

static_assert(!std::is_aggregate_v<Duration>);
static_assert(!std::is_constructible_v<Duration, std::int64_t>);
static_assert(!std::is_convertible_v<std::int64_t, Duration>);

} // namespace trivial::time

#endif // TRIVIAL_CORE_TIME_DURATION_H

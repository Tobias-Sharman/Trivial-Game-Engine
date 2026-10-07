#ifndef TRIVIAL_CORE_TIME_DURATION_H
#define TRIVIAL_CORE_TIME_DURATION_H

#include <cstdint>
#include <type_traits>

#include <trivial/core/assert.h>
#include <trivial/core/config.h>
#include <trivial/core/time/time_constants.h>

#if TRIVIAL_ENABLE_ASSERTS
#include <limits>
#endif // TRIVIAL_ENABLE_ASSERTS

namespace trivial::time {

struct Duration {
	[[nodiscard]] static constexpr Duration fromNanoseconds(std::int64_t nanoseconds) noexcept {
		Duration result;
		result.m_nanoseconds = nanoseconds;
		return result;
	}

	[[nodiscard]] constexpr std::int64_t count() const noexcept { return m_nanoseconds; }

	[[nodiscard]] constexpr std::int64_t toMicroseconds() const noexcept {
		return m_nanoseconds / TRIVIAL_TIME_NANOSECONDS_PER_MICROSECOND;
	}

	[[nodiscard]] constexpr std::int64_t toMilliseconds() const noexcept {
		return m_nanoseconds / TRIVIAL_TIME_NANOSECONDS_PER_MILLISECOND;
	}

	[[nodiscard]] constexpr std::int64_t toMillisecondsCeil() const noexcept {
		const std::int64_t kWhole = m_nanoseconds / TRIVIAL_TIME_NANOSECONDS_PER_MILLISECOND;
		return m_nanoseconds % TRIVIAL_TIME_NANOSECONDS_PER_MILLISECOND > 0 ? kWhole + 1 : kWhole;
	}

	[[nodiscard]] constexpr std::int64_t toSeconds() const noexcept {
		return m_nanoseconds / TRIVIAL_TIME_NANOSECONDS_PER_SECOND;
	}

	[[nodiscard]] constexpr double toSecondsDouble() const noexcept {
		return static_cast<double>(m_nanoseconds) / static_cast<double>(TRIVIAL_TIME_NANOSECONDS_PER_SECOND);
	}

	[[nodiscard]] constexpr Duration operator+() const noexcept { return *this; }
	[[nodiscard]] constexpr Duration operator-() const noexcept {
		Duration result = *this;
		result.m_nanoseconds = -m_nanoseconds;
		return result;
	}

	[[nodiscard]] constexpr Duration operator+(Duration rhs) const noexcept {
		Duration result = *this;
		result.m_nanoseconds += rhs.m_nanoseconds;
		return result;
	}

	[[nodiscard]] constexpr Duration operator-(Duration rhs) const noexcept {
		Duration result = *this;
		result.m_nanoseconds -= rhs.m_nanoseconds;
		return result;
	}

	[[nodiscard]] constexpr Duration operator*(std::int64_t scalar) const noexcept {
		Duration result = *this;
		result.m_nanoseconds *= scalar;
		return result;
	}

	[[nodiscard]] constexpr Duration operator/(std::int64_t scalar) const noexcept {
		TRIVIAL_ASSUME(scalar != 0);
		Duration result = *this;
		result.m_nanoseconds /= scalar;
		return result;
	}

	[[nodiscard]] constexpr std::int64_t operator/(Duration rhs) const noexcept {
		TRIVIAL_ASSUME(rhs.m_nanoseconds != 0);
		return m_nanoseconds / rhs.m_nanoseconds;
	}

	[[nodiscard]] constexpr Duration operator%(Duration rhs) const noexcept {
		TRIVIAL_ASSUME(rhs.m_nanoseconds != 0);
		Duration result = *this;
		result.m_nanoseconds %= rhs.m_nanoseconds;
		return result;
	}

	constexpr Duration& operator+=(Duration rhs) noexcept {
		m_nanoseconds += rhs.m_nanoseconds;
		return *this;
	}

	constexpr Duration& operator-=(Duration rhs) noexcept {
		m_nanoseconds -= rhs.m_nanoseconds;
		return *this;
	}

	constexpr Duration& operator*=(std::int64_t scalar) noexcept {
		m_nanoseconds *= scalar;
		return *this;
	}

	constexpr Duration& operator/=(std::int64_t scalar) noexcept {
		TRIVIAL_ASSUME(scalar != 0);
		m_nanoseconds /= scalar;
		return *this;
	}

	constexpr Duration& operator%=(Duration rhs) noexcept {
		TRIVIAL_ASSUME(rhs.m_nanoseconds != 0);
		m_nanoseconds %= rhs.m_nanoseconds;
		return *this;
	}

	[[nodiscard]] constexpr bool operator==(const Duration&) const noexcept = default;

	[[nodiscard]] constexpr bool operator<(Duration rhs) const noexcept { return m_nanoseconds < rhs.m_nanoseconds; }
	[[nodiscard]] constexpr bool operator<=(Duration rhs) const noexcept { return m_nanoseconds <= rhs.m_nanoseconds; }
	[[nodiscard]] constexpr bool operator>(Duration rhs) const noexcept { return m_nanoseconds > rhs.m_nanoseconds; }
	[[nodiscard]] constexpr bool operator>=(Duration rhs) const noexcept { return m_nanoseconds >= rhs.m_nanoseconds; }

private:
	std::int64_t m_nanoseconds = 0;
};

[[nodiscard]] constexpr Duration operator*(std::int64_t scalar, Duration duration) noexcept {
	return duration * scalar;
}

[[nodiscard]] constexpr Duration nanoseconds(std::int64_t count) noexcept {
	return Duration::fromNanoseconds(count);
}

[[nodiscard]] constexpr Duration microseconds(std::int64_t count) noexcept {
	TRIVIAL_ASSERT(count <= std::numeric_limits<std::int64_t>::max() / TRIVIAL_TIME_NANOSECONDS_PER_MICROSECOND);
	TRIVIAL_ASSERT(count >= std::numeric_limits<std::int64_t>::min() / TRIVIAL_TIME_NANOSECONDS_PER_MICROSECOND);
	return Duration::fromNanoseconds(count * TRIVIAL_TIME_NANOSECONDS_PER_MICROSECOND);
}

[[nodiscard]] constexpr Duration milliseconds(std::int64_t count) noexcept {
	TRIVIAL_ASSERT(count <= std::numeric_limits<std::int64_t>::max() / TRIVIAL_TIME_NANOSECONDS_PER_MILLISECOND);
	TRIVIAL_ASSERT(count >= std::numeric_limits<std::int64_t>::min() / TRIVIAL_TIME_NANOSECONDS_PER_MILLISECOND);
	return Duration::fromNanoseconds(count * TRIVIAL_TIME_NANOSECONDS_PER_MILLISECOND);
}

[[nodiscard]] constexpr Duration seconds(std::int64_t count) noexcept {
	TRIVIAL_ASSERT(count <= std::numeric_limits<std::int64_t>::max() / TRIVIAL_TIME_NANOSECONDS_PER_SECOND);
	TRIVIAL_ASSERT(count >= std::numeric_limits<std::int64_t>::min() / TRIVIAL_TIME_NANOSECONDS_PER_SECOND);
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

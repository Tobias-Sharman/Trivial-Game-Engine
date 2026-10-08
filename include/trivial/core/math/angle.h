#ifndef TRIVIAL_CORE_MATH_ANGLE_H
#define TRIVIAL_CORE_MATH_ANGLE_H

#include <type_traits>

#include <trivial/core/assert.h>
#include <trivial/core/math/concepts.h>
#include <trivial/core/math/math_constants.h>

namespace trivial::math {

template <FloatingPoint T>
struct Angle {
	[[nodiscard]] static constexpr Angle fromRadians(T radians) noexcept {
		Angle result;
		result.m_radians = radians;
		return result;
	}

	[[nodiscard]] static constexpr Angle fromDegrees(T degrees) noexcept {
		return fromRadians(degrees * constants::g_kDegreesToRadians<T>);
	}

	[[nodiscard]] constexpr T radians() const noexcept { return m_radians; }
	[[nodiscard]] constexpr T degrees() const noexcept { return m_radians * constants::g_kRadiansToDegrees<T>; }

	[[nodiscard]] constexpr Angle operator+() const noexcept { return *this; }
	[[nodiscard]] constexpr Angle operator-() const noexcept { return fromRadians(-m_radians); }

	[[nodiscard]] constexpr Angle operator+(Angle rhs) const noexcept { return fromRadians(m_radians + rhs.m_radians); }
	[[nodiscard]] constexpr Angle operator-(Angle rhs) const noexcept { return fromRadians(m_radians - rhs.m_radians); }
	[[nodiscard]] constexpr Angle operator*(T scalar) const noexcept { return fromRadians(m_radians * scalar); }
	[[nodiscard]] constexpr Angle operator/(T scalar) const noexcept {
		TRIVIAL_ASSUME(scalar != T{});

		return fromRadians(m_radians / scalar);
	}

	constexpr Angle& operator+=(Angle rhs) noexcept {
		m_radians += rhs.m_radians;
		return *this;
	}

	constexpr Angle& operator-=(Angle rhs) noexcept {
		m_radians -= rhs.m_radians;
		return *this;
	}

	constexpr Angle& operator*=(T scalar) noexcept {
		m_radians *= scalar;
		return *this;
	}

	constexpr Angle& operator/=(T scalar) noexcept {
		TRIVIAL_ASSUME(scalar != T{});

		m_radians /= scalar;
		return *this;
	}

	[[nodiscard]] constexpr bool operator==(const Angle&) const noexcept = default;

	[[nodiscard]] constexpr bool nearlyEqual(const Angle& rhs, T epsilon) const noexcept {
		T difference = m_radians > rhs.m_radians ? m_radians - rhs.m_radians : rhs.m_radians - m_radians;

		return difference <= epsilon;
	}

private:
	T m_radians{};
};

template <FloatingPoint T>
[[nodiscard]] constexpr Angle<T> operator*(T scalar, Angle<T> angle) noexcept {
	return angle * scalar;
}

template <FloatingPoint T>
[[nodiscard]] constexpr bool nearlyEqual(const Angle<T>& lhs, const Angle<T>& rhs, T epsilon) noexcept {
	return lhs.nearlyEqual(rhs, epsilon);
}

using Anglef = Angle<float>;
using Angled = Angle<double>;

static_assert(sizeof(Anglef) == sizeof(float));
static_assert(sizeof(Angled) == sizeof(double));

static_assert(alignof(Anglef) == alignof(float));
static_assert(alignof(Angled) == alignof(double));

static_assert(std::is_trivially_copyable_v<Anglef>);
static_assert(std::is_trivially_copyable_v<Angled>);

static_assert(std::is_standard_layout_v<Anglef>);
static_assert(std::is_standard_layout_v<Angled>);

static_assert(!std::is_convertible_v<float, Anglef>);
static_assert(!std::is_convertible_v<double, Angled>);

} // namespace trivial::math

#endif // TRIVIAL_CORE_MATH_ANGLE_H

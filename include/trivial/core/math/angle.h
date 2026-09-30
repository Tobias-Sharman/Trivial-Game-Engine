#ifndef TRIVIAL_CORE_MATH_ANGLE_H
#define TRIVIAL_CORE_MATH_ANGLE_H

#include <trivial/core/math/concepts.h>
#include <trivial/core/math/math_constants.h>

namespace trivial::math {

template <FloatingPoint T>
class Angle {
public:
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

	[[nodiscard]] constexpr Angle operator+(Angle other) const noexcept {
		return fromRadians(m_radians + other.m_radians);
	}
	[[nodiscard]] constexpr Angle operator-(Angle other) const noexcept {
		return fromRadians(m_radians - other.m_radians);
	}
	[[nodiscard]] constexpr Angle operator*(T scalar) const noexcept { return fromRadians(m_radians * scalar); }
	[[nodiscard]] constexpr Angle operator/(T scalar) const noexcept { return fromRadians(m_radians / scalar); }

	constexpr Angle& operator+=(Angle other) noexcept {
		m_radians += other.m_radians;
		return *this;
	}

	constexpr Angle& operator-=(Angle other) noexcept {
		m_radians -= other.m_radians;
		return *this;
	}

	constexpr Angle& operator*=(T scalar) noexcept {
		m_radians *= scalar;
		return *this;
	}

	constexpr Angle& operator/=(T scalar) noexcept {
		m_radians /= scalar;
		return *this;
	}

	[[nodiscard]] constexpr bool operator==(const Angle&) const = default;

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

} // namespace trivial::math

#endif // TRIVIAL_CORE_MATH_ANGLE_H

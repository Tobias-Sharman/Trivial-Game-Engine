
#ifndef TRIVIAL_CORE_MATH_AFFINE2_H
#define TRIVIAL_CORE_MATH_AFFINE2_H

// | a  b  tx |
// | c  d  ty |
// | 0  0   1 |

#include <cmath>
#include <type_traits>

#include <trivial/core/math/angle.h>
#include <trivial/core/math/concepts.h>
#include <trivial/core/math/vec2.h>

namespace trivial::math {

template <FloatingPoint T>
struct Affine2 {
	T a{};
	T b{};
	T tx{};
	T c{};
	T d{};
	T ty{};

	[[nodiscard]] static constexpr Affine2 identity() noexcept { return {T{1}, T{}, T{}, T{}, T{1}, T{}}; }

	[[nodiscard]] static constexpr Affine2 translation(const Vec2<T>& translation) noexcept {
		return {T{1}, T{}, translation.x, T{}, T{1}, translation.y};
	}

	[[nodiscard]] static constexpr Affine2 scale(const Vec2<T>& scale) noexcept {
		return {scale.x, T{}, T{}, T{}, scale.y, T{}};
	}

	[[nodiscard]] static constexpr Affine2 scale(T scale) noexcept { return {scale, T{}, T{}, T{}, scale, T{}}; }

	[[nodiscard]] static Affine2 rotation(Angle<T> angle) noexcept
	    requires FloatingPoint<T>
	{
		T cosAngle = std::cos(angle.radians());
		T sinAngle = std::sin(angle.radians());

		return {cosAngle, -sinAngle, T{}, sinAngle, cosAngle, T{}};
	}

	[[nodiscard]] constexpr Vec2<T> transformPoint(const Vec2<T>& point) const noexcept {
		return {a * point.x + b * point.y + tx, c * point.x + d * point.y + ty};
	}

	[[nodiscard]] constexpr Vec2<T> transformVector(const Vec2<T>& vector) const noexcept {
		return {a * vector.x + b * vector.y, c * vector.x + d * vector.y};
	}

	[[nodiscard]] constexpr T determinant() const noexcept { return a * d - b * c; }

	[[nodiscard]] constexpr Affine2 operator*(const Affine2& rhs) const noexcept {
		return {a * rhs.a + b * rhs.c,
		        a * rhs.b + b * rhs.d,
		        a * rhs.tx + b * rhs.ty + tx,

		        c * rhs.a + d * rhs.c,
		        c * rhs.b + d * rhs.d,
		        c * rhs.tx + d * rhs.ty + ty};
	}

	constexpr Affine2& operator*=(const Affine2& rhs) noexcept {
		*this = *this * rhs;
		return *this;
	}

	[[nodiscard]] constexpr bool operator==(const Affine2& rhs) const noexcept = default;

	[[nodiscard]] constexpr bool nearlyEqual(const Affine2& rhs, T epsilon) const noexcept
	    requires FloatingPoint<T>
	{
		T da = a > rhs.a ? a - rhs.a : rhs.a - a;
		T db = b > rhs.b ? b - rhs.b : rhs.b - b;
		T dtx = tx > rhs.tx ? tx - rhs.tx : rhs.tx - tx;
		T dc = c > rhs.c ? c - rhs.c : rhs.c - c;
		T dd = d > rhs.d ? d - rhs.d : rhs.d - d;
		T dty = ty > rhs.ty ? ty - rhs.ty : rhs.ty - ty;

		return da <= epsilon && db <= epsilon && dtx <= epsilon && dc <= epsilon && dd <= epsilon && ty <= epsilon;
	}
};

template <FloatingPoint T>
[[nodiscard]] constexpr bool nearlyEqual(const Affine2<T>& lhs, const Affine2<T>& rhs, T epsilon) noexcept {
	return lhs.nearlyEqual(rhs, epsilon);
}

using Affine2f = Affine2<float>;
using Affine2d = Affine2<double>;

static_assert(sizeof(Affine2f) == sizeof(float) * 6);  // NOLINT(readability-magic-numbers)
static_assert(sizeof(Affine2d) == sizeof(double) * 6); // NOLINT(readability-magic-numbers)

static_assert(alignof(Affine2f) == alignof(float));
static_assert(alignof(Affine2d) == alignof(double));

static_assert(std::is_trivially_copyable_v<Affine2f>);
static_assert(std::is_trivially_copyable_v<Affine2d>);

static_assert(std::is_standard_layout_v<Affine2f>);
static_assert(std::is_standard_layout_v<Affine2d>);

} // namespace trivial::math

#endif // TRIVIAL_CORE_MATH_AFFINE2_H

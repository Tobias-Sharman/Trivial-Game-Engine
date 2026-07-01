#ifndef TRIVIAL_CORE_MATH_MAT2X3_H
#define TRIVIAL_CORE_MATH_MAT2X3_H

#include <cmath>
#include <type_traits>

#include <trivial/core/math/concepts.h>
#include <trivial/core/math/vec2.h>

namespace trivial::math {

template <Arithmetic T>
inline constexpr auto g_kMat2x3Size = sizeof(T) * 6;

template <Arithmetic T>
struct Mat2x3 {
	T a{};
	T b{};
	T tx{};

	T c{};
	T d{};
	T ty{};

	constexpr Mat2x3() noexcept = default;

	constexpr Mat2x3(T a, T b, T tx, T c, T d, T ty) noexcept
	    : a(a)
	    , b(b)
	    , tx(tx)
	    , c(c)
	    , d(d)
	    , ty(ty) {}

	[[nodiscard]] static constexpr Mat2x3 identity() noexcept { return {T(1), T(0), T(0), T(0), T(1), T(0)}; }

	[[nodiscard]] static constexpr Mat2x3 translation(Vec2<T> v) noexcept { return {T(1), T(0), v.x, T(0), T(1), v.y}; }

	[[nodiscard]] static constexpr Mat2x3 scale(Vec2<T> v) noexcept { return {v.x, T(0), T(0), T(0), v.y, T(0)}; }

	[[nodiscard]] static Mat2x3 rotation(T radians) noexcept
	    requires std::floating_point<T>
	{
		T cosAngle = std::cos(radians);
		T sinAngle = std::sin(radians);

		return {cosAngle, -sinAngle, T(0), sinAngle, cosAngle, T(0)};
	}

	[[nodiscard]] constexpr Vec2<T> transformPoint(Vec2<T> point) const noexcept {
		return {a * point.x + b * point.y + tx, c * point.x + d * point.y + ty};
	}

	[[nodiscard]] constexpr Vec2<T> transformVector(Vec2<T> vector) const noexcept {
		return {a * vector.x + b * vector.y, c * vector.x + d * vector.y};
	}

	[[nodiscard]] constexpr bool operator==(const Mat2x3& rhs) const noexcept = default;
};

template <Arithmetic T>
[[nodiscard]] constexpr Vec2<T> operator*(Mat2x3<T> m, Vec2<T> point) noexcept {
	return m.transformPoint(point);
}

template <Arithmetic T>
[[nodiscard]] constexpr Mat2x3<T> operator*(Mat2x3<T> lhs, Mat2x3<T> rhs) noexcept {
	return {lhs.a * rhs.a + lhs.b * rhs.c,
	        lhs.a * rhs.b + lhs.b * rhs.d,
	        lhs.a * rhs.tx + lhs.b * rhs.ty + lhs.tx,

	        lhs.c * rhs.a + lhs.d * rhs.c,
	        lhs.c * rhs.b + lhs.d * rhs.d,
	        lhs.c * rhs.tx + lhs.d * rhs.ty + lhs.ty};
}

template <Arithmetic T>
constexpr Mat2x3<T>& operator*=(Mat2x3<T>& lhs, Mat2x3<T> rhs) noexcept {
	lhs = lhs * rhs;
	return lhs;
}

template <std::floating_point T>
[[nodiscard]] constexpr bool nearlyEqual(Mat2x3<T> lhs, Mat2x3<T> rhs, T epsilon) noexcept {
	T da = lhs.a > rhs.a ? lhs.a - rhs.a : rhs.a - lhs.a;
	T db = lhs.b > rhs.b ? lhs.b - rhs.b : rhs.b - lhs.b;
	T dtx = lhs.tx > rhs.tx ? lhs.tx - rhs.tx : rhs.tx - lhs.tx;

	T dc = lhs.c > rhs.c ? lhs.c - rhs.c : rhs.c - lhs.c;
	T dd = lhs.d > rhs.d ? lhs.d - rhs.d : rhs.d - lhs.d;
	T dty = lhs.ty > rhs.ty ? lhs.ty - rhs.ty : rhs.ty - lhs.ty;

	return da <= epsilon && db <= epsilon && dtx <= epsilon && dc <= epsilon && dd <= epsilon && dty <= epsilon;
}

using Mat2x3f = Mat2x3<float>;
using Mat2x3d = Mat2x3<double>;
using Mat2x3i = Mat2x3<int>;

static_assert(sizeof(Mat2x3f) == g_kMat2x3Size<float>);
static_assert(sizeof(Mat2x3d) == g_kMat2x3Size<double>);
static_assert(sizeof(Mat2x3i) == g_kMat2x3Size<int>);

static_assert(alignof(Mat2x3f) == alignof(float));
static_assert(alignof(Mat2x3d) == alignof(double));
static_assert(alignof(Mat2x3i) == alignof(int));

static_assert(std::is_trivially_copyable_v<Mat2x3f>);
static_assert(std::is_trivially_copyable_v<Mat2x3d>);
static_assert(std::is_trivially_copyable_v<Mat2x3i>);

static_assert(std::is_standard_layout_v<Mat2x3f>);
static_assert(std::is_standard_layout_v<Mat2x3d>);
static_assert(std::is_standard_layout_v<Mat2x3i>);

} // namespace trivial::math

#endif // TRIVIAL_CORE_MATH_MAT2X3_H

#ifndef TRIVIAL_CORE_MATH_VEC2_H
#define TRIVIAL_CORE_MATH_VEC2_H

#include <cmath>
#include <type_traits>

#include <trivial/core/math/concepts.h>

namespace trivial::math {

template <Arithmetic T>
inline constexpr auto g_kVec2Size = sizeof(T) * 2;

template <Arithmetic T>
struct Vec2 {
	T x{};
	T y{};

	constexpr Vec2() noexcept = default;

	constexpr Vec2(T x, T y) noexcept
	    : x(x)
	    , y(y) {}

	[[nodiscard]] constexpr T& operator[](int index) noexcept {
		switch (index) {
			default:
			case 0:
				return x;
			case 1:
				return y;
		}
	}

	[[nodiscard]] constexpr const T& operator[](int index) const noexcept {
		switch (index) {
			default:
			case 0:
				return x;
			case 1:
				return y;
		}
	}

	[[nodiscard]] constexpr Vec2 operator+() const noexcept { return *this; }
	[[nodiscard]] constexpr Vec2 operator-() const noexcept { return {-x, -y}; }

	[[nodiscard]] constexpr Vec2 operator+(Vec2 rhs) const noexcept { return {x + rhs.x, y + rhs.y}; }
	[[nodiscard]] constexpr Vec2 operator-(Vec2 rhs) const noexcept { return {x - rhs.x, y - rhs.y}; }
	[[nodiscard]] constexpr Vec2 operator*(Vec2 rhs) const noexcept { return {x * rhs.x, y * rhs.y}; }
	[[nodiscard]] constexpr Vec2 operator/(Vec2 rhs) const noexcept { return {x / rhs.x, y / rhs.y}; }

	[[nodiscard]] constexpr Vec2 operator*(T scalar) const noexcept { return {x * scalar, y * scalar}; }
	[[nodiscard]] constexpr Vec2 operator/(T scalar) const noexcept { return {x / scalar, y / scalar}; }

	constexpr Vec2& operator+=(Vec2 rhs) noexcept {
		x += rhs.x;
		y += rhs.y;
		return *this;
	}

	constexpr Vec2& operator-=(Vec2 rhs) noexcept {
		x -= rhs.x;
		y -= rhs.y;
		return *this;
	}

	constexpr Vec2& operator*=(Vec2 rhs) noexcept {
		x *= rhs.x;
		y *= rhs.y;
		return *this;
	}

	constexpr Vec2& operator/=(Vec2 rhs) noexcept {
		x /= rhs.x;
		y /= rhs.y;
		return *this;
	}

	constexpr Vec2& operator*=(T scalar) noexcept {
		x *= scalar;
		y *= scalar;
		return *this;
	}

	constexpr Vec2& operator/=(T scalar) noexcept {
		x /= scalar;
		y /= scalar;
		return *this;
	}

	[[nodiscard]] constexpr bool operator==(const Vec2& rhs) const noexcept = default;
};

template <Arithmetic T>
[[nodiscard]] constexpr Vec2<T> operator*(T scalar, Vec2<T> v) noexcept {
	return v * scalar;
}

template <Arithmetic T>
[[nodiscard]] constexpr T dot(Vec2<T> a, Vec2<T> b) noexcept {
	return a.x * b.x + a.y * b.y;
}

template <Arithmetic T>
[[nodiscard]] constexpr T cross(Vec2<T> a, Vec2<T> b) noexcept {
	return a.x * b.y - a.y * b.x;
}

template <Arithmetic T>
[[nodiscard]] constexpr T lengthSquared(Vec2<T> v) noexcept {
	return dot(v, v);
}

template <std::floating_point T>
[[nodiscard]] T length(Vec2<T> v) noexcept {
	return std::sqrt(lengthSquared(v));
}

template <std::floating_point T>
[[nodiscard]] T robustLength(Vec2<T> v) noexcept {
	return std::hypot(v.x, v.y);
}

template <std::floating_point T>
[[nodiscard]] Vec2<T> normalise(Vec2<T> v) noexcept {
	return v / length(v);
}

template <std::floating_point T>
[[nodiscard]] Vec2<T> normaliseOrZero(Vec2<T> v) noexcept {
	T len = length(v);
	return len != T(0) ? v / len : Vec2<T>{};
}

template <Arithmetic T>
[[nodiscard]] constexpr Vec2<T> perpLeft(Vec2<T> v) noexcept {
	return {-v.y, v.x};
}

template <Arithmetic T>
[[nodiscard]] constexpr Vec2<T> perpRight(Vec2<T> v) noexcept {
	return {v.y, -v.x};
}

template <std::floating_point T>
[[nodiscard]] constexpr Vec2<T> lerp(Vec2<T> a, Vec2<T> b, T t) noexcept {
	return a + (b - a) * t;
}

template <std::floating_point T>
[[nodiscard]] constexpr bool nearlyEqual(Vec2<T> a, Vec2<T> b, T epsilon) noexcept {
	T dx = a.x > b.x ? a.x - b.x : b.x - a.x;
	T dy = a.y > b.y ? a.y - b.y : b.y - a.y;
	return dx <= epsilon && dy <= epsilon;
}

using Vec2f = Vec2<float>;
using Vec2d = Vec2<double>;
using Vec2i = Vec2<int>;

static_assert(sizeof(Vec2f) == g_kVec2Size<float>);
static_assert(sizeof(Vec2d) == g_kVec2Size<double>);
static_assert(sizeof(Vec2i) == g_kVec2Size<int>);

static_assert(alignof(Vec2f) == alignof(float));
static_assert(alignof(Vec2d) == alignof(double));
static_assert(alignof(Vec2i) == alignof(int));

static_assert(std::is_trivially_copyable_v<Vec2f>);
static_assert(std::is_trivially_copyable_v<Vec2d>);
static_assert(std::is_trivially_copyable_v<Vec2i>);

static_assert(std::is_standard_layout_v<Vec2f>);
static_assert(std::is_standard_layout_v<Vec2d>);
static_assert(std::is_standard_layout_v<Vec2i>);

} // namespace trivial::math

#endif // TRIVIAL_CORE_MATH_VEC2_H

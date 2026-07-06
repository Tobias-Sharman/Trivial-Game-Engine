#ifndef TRIVIAL_CORE_MATH_VEC2_H
#define TRIVIAL_CORE_MATH_VEC2_H

#include <cmath>
#include <cstddef>
#include <type_traits>

#include <trivial/core/assert.h>
#include <trivial/core/math/concepts.h>

namespace trivial::math {

template <Arithmetic T>
struct Vec2 {
	T x{};
	T y{};

	[[nodiscard]] constexpr T& operator[](std::size_t index) noexcept {
		TRIVIAL_ASSERT(index < 2);

		switch (index) {
			default:
			case 0:
				return x;
			case 1:
				return y;
		}
	}

	[[nodiscard]] constexpr const T& operator[](std::size_t index) const noexcept {
		TRIVIAL_ASSERT(index < 2);

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

	[[nodiscard]] constexpr T dot(Vec2 rhs) const noexcept { return (x * rhs.x) + (y * rhs.y); }

	[[nodiscard]] constexpr T cross(Vec2 rhs) const noexcept { return (x * rhs.y) - (y * rhs.x); }

	[[nodiscard]] constexpr T lengthSquared() const noexcept { return dot(*this); }

	[[nodiscard]] T length() const noexcept
	    requires FloatingPoint<T>
	{
		return std::sqrt(lengthSquared());
	}

	[[nodiscard]] T robustLength() const noexcept
	    requires FloatingPoint<T>
	{
		return std::hypot(x, y);
	}

	[[nodiscard]] Vec2 normalised() const noexcept
	    requires FloatingPoint<T>
	{
		T vectorLength = length();

		TRIVIAL_ASSERT(vectorLength != T{});

		return *this / vectorLength;
	}

	[[nodiscard]] Vec2 normalisedOrZero() const noexcept
	    requires FloatingPoint<T>
	{
		T vectorLength = length();

		if (vectorLength == T{}) {
			return {};
		}

		return *this / vectorLength;
	}

	[[nodiscard]] constexpr Vec2 perpLeft() const noexcept { return {-y, x}; }
	[[nodiscard]] constexpr Vec2 perpRight() const noexcept { return {y, -x}; }

	[[nodiscard]] constexpr Vec2 lerp(Vec2 rhs, T t) const noexcept
	    requires FloatingPoint<T>
	{
		return *this + ((rhs - *this) * t);
	}

	[[nodiscard]] constexpr bool operator==(const Vec2&) const noexcept = default;

	[[nodiscard]] constexpr bool nearlyEqual(Vec2 rhs, T epsilon) const noexcept
	    requires FloatingPoint<T>
	{
		T dx = x > rhs.x ? x - rhs.x : rhs.x - x;
		T dy = y > rhs.y ? y - rhs.y : rhs.y - y;

		return dx <= epsilon && dy <= epsilon;
	}
};

template <Arithmetic T>
[[nodiscard]] constexpr Vec2<T> operator*(T scalar, Vec2<T> vector) noexcept {
	return vector * scalar;
}

template <Arithmetic T>
[[nodiscard]] constexpr T dot(Vec2<T> lhs, Vec2<T> rhs) noexcept {
	return lhs.dot(rhs);
}

template <Arithmetic T>
[[nodiscard]] constexpr T cross(Vec2<T> lhs, Vec2<T> rhs) noexcept {
	return lhs.cross(rhs);
}

template <FloatingPoint T>
[[nodiscard]] constexpr Vec2<T> lerp(Vec2<T> lhs, Vec2<T> rhs, T t) noexcept {
	return lhs.lerp(rhs, t);
}

template <FloatingPoint T>
[[nodiscard]] constexpr bool nearlyEqual(Vec2<T> lhs, Vec2<T> rhs, T epsilon) noexcept {
	return lhs.nearlyEqual(rhs, epsilon);
}

using Vec2i = Vec2<int>;
using Vec2f = Vec2<float>;
using Vec2d = Vec2<double>;

static_assert(sizeof(Vec2i) == sizeof(int) * 2);
static_assert(sizeof(Vec2f) == sizeof(float) * 2);
static_assert(sizeof(Vec2d) == sizeof(double) * 2);

static_assert(alignof(Vec2i) == alignof(int));
static_assert(alignof(Vec2f) == alignof(float));
static_assert(alignof(Vec2d) == alignof(double));

static_assert(std::is_trivially_copyable_v<Vec2i>);
static_assert(std::is_trivially_copyable_v<Vec2f>);
static_assert(std::is_trivially_copyable_v<Vec2d>);

static_assert(std::is_standard_layout_v<Vec2i>);
static_assert(std::is_standard_layout_v<Vec2f>);
static_assert(std::is_standard_layout_v<Vec2d>);

} // namespace trivial::math

#endif // TRIVIAL_CORE_MATH_VEC2_H

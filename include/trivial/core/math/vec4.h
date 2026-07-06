#ifndef TRIVIAL_CORE_MATH_VEC4_H
#define TRIVIAL_CORE_MATH_VEC4_H

#include <cmath>
#include <cstddef>
#include <type_traits>

#include <trivial/core/assert.h>
#include <trivial/core/math/concepts.h>

namespace trivial::math {

template <Arithmetic T>
struct Vec4 {
	T x{};
	T y{};
	T z{};
	T w{};

	[[nodiscard]] constexpr T& operator[](std::size_t index) noexcept {
		TRIVIAL_ASSERT(index < 4);

		switch (index) {
			default:
			case 0:
				return x;
			case 1:
				return y;
			case 2:
				return z;
			case 3:
				return w;
		}
	}

	[[nodiscard]] constexpr const T& operator[](std::size_t index) const noexcept {
		TRIVIAL_ASSERT(index < 4);

		switch (index) {
			default:
			case 0:
				return x;
			case 1:
				return y;
			case 2:
				return z;
			case 3:
				return w;
		}
	}

	[[nodiscard]] constexpr Vec4 operator+() const noexcept { return *this; }
	[[nodiscard]] constexpr Vec4 operator-() const noexcept { return {-x, -y, -z, -w}; }

	[[nodiscard]] constexpr Vec4 operator+(Vec4 rhs) const noexcept {
		return {x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w};
	}
	[[nodiscard]] constexpr Vec4 operator-(Vec4 rhs) const noexcept {
		return {x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w};
	}
	[[nodiscard]] constexpr Vec4 operator*(Vec4 rhs) const noexcept {
		return {x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w};
	}
	[[nodiscard]] constexpr Vec4 operator/(Vec4 rhs) const noexcept {
		return {x / rhs.x, y / rhs.y, z / rhs.z, w / rhs.w};
	}

	[[nodiscard]] constexpr Vec4 operator*(T scalar) const noexcept {
		return {x * scalar, y * scalar, z * scalar, w * scalar};
	}
	[[nodiscard]] constexpr Vec4 operator/(T scalar) const noexcept {
		return {x / scalar, y / scalar, z / scalar, w / scalar};
	}

	constexpr Vec4& operator+=(Vec4 rhs) noexcept {
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
		w += rhs.w;
		return *this;
	}

	constexpr Vec4& operator-=(Vec4 rhs) noexcept {
		x -= rhs.x;
		y -= rhs.y;
		z -= rhs.z;
		w -= rhs.w;
		return *this;
	}

	constexpr Vec4& operator*=(Vec4 rhs) noexcept {
		x *= rhs.x;
		y *= rhs.y;
		z *= rhs.z;
		w *= rhs.w;
		return *this;
	}

	constexpr Vec4& operator/=(Vec4 rhs) noexcept {
		x /= rhs.x;
		y /= rhs.y;
		z /= rhs.z;
		w /= rhs.w;
		return *this;
	}

	constexpr Vec4& operator*=(T scalar) noexcept {
		x *= scalar;
		y *= scalar;
		z *= scalar;
		w *= scalar;
		return *this;
	}

	constexpr Vec4& operator/=(T scalar) noexcept {
		x /= scalar;
		y /= scalar;
		z /= scalar;
		w /= scalar;
		return *this;
	}

	[[nodiscard]] constexpr T dot(Vec4 rhs) const noexcept {
		return (x * rhs.x) + (y * rhs.y) + (z * rhs.z) + (w * rhs.w);
	}

	[[nodiscard]] constexpr T lengthSquared() const noexcept { return dot(*this); }

	[[nodiscard]] T length() const noexcept
	    requires FloatingPoint<T>
	{
		return std::sqrt(lengthSquared());
	}

	[[nodiscard]] T robustLength() const noexcept
	    requires FloatingPoint<T>
	{
		return std::hypot(std::hypot(x, y), std::hypot(z, w));
		// TODO: Make a custom function for this to avoid duplicate operations
	}

	[[nodiscard]] Vec4 normalised() const noexcept
	    requires FloatingPoint<T>
	{
		T vectorLength = length();

		TRIVIAL_ASSERT(vectorLength != T{});

		return *this / vectorLength;
	}

	[[nodiscard]] Vec4 normalisedOrZero() const noexcept
	    requires FloatingPoint<T>
	{
		T vectorLength = length();

		if (vectorLength == T{}) {
			return {};
		}

		return *this / vectorLength;
	}

	// TODO: perp when I setle on a convention for direction

	[[nodiscard]] constexpr Vec4 lerp(Vec4 rhs, T t) const noexcept
	    requires FloatingPoint<T>
	{
		return *this + ((rhs - *this) * t);
	}

	[[nodiscard]] constexpr bool operator==(const Vec4&) const noexcept = default;

	[[nodiscard]] constexpr bool nearlyEqual(Vec4 rhs, T epsilon) const noexcept
	    requires FloatingPoint<T>
	{
		T dx = x > rhs.x ? x - rhs.x : rhs.x - x;
		T dy = y > rhs.y ? y - rhs.y : rhs.y - y;
		T dz = z > rhs.z ? z - rhs.z : rhs.z - z;
		T dw = w > rhs.w ? w - rhs.w : rhs.w - w;

		return dx <= epsilon && dy <= epsilon && dz <= epsilon && dw <= epsilon;
	}
};

template <Arithmetic T>
[[nodiscard]] constexpr Vec4<T> operator*(T scalar, Vec4<T> vector) noexcept {
	return vector * scalar;
}

template <Arithmetic T>
[[nodiscard]] constexpr T dot(Vec4<T> lhs, Vec4<T> rhs) noexcept {
	return lhs.dot(rhs);
}

template <FloatingPoint T>
[[nodiscard]] constexpr Vec4<T> lerp(Vec4<T> lhs, Vec4<T> rhs, T t) noexcept {
	return lhs.lerp(rhs, t);
}

template <FloatingPoint T>
[[nodiscard]] constexpr bool nearlyEqual(Vec4<T> lhs, Vec4<T> rhs, T epsilon) noexcept {
	return lhs.nearlyEqual(rhs, epsilon);
}

using Vec4i = Vec4<int>;
using Vec4f = Vec4<float>;
using Vec4d = Vec4<double>;

static_assert(sizeof(Vec4i) == sizeof(int) * 4);
static_assert(sizeof(Vec4f) == sizeof(float) * 4);
static_assert(sizeof(Vec4d) == sizeof(double) * 4);

static_assert(alignof(Vec4i) == alignof(int));
static_assert(alignof(Vec4f) == alignof(float));
static_assert(alignof(Vec4d) == alignof(double));

static_assert(std::is_trivially_copyable_v<Vec4i>);
static_assert(std::is_trivially_copyable_v<Vec4f>);
static_assert(std::is_trivially_copyable_v<Vec4d>);

static_assert(std::is_standard_layout_v<Vec4i>);
static_assert(std::is_standard_layout_v<Vec4f>);
static_assert(std::is_standard_layout_v<Vec4d>);

} // namespace trivial::math

#endif // TRIVIAL_CORE_MATH_VEC4_H

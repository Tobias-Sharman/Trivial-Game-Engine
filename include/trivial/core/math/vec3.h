#ifndef TRIVIAL_CORE_MATH_VEC3_H
#define TRIVIAL_CORE_MATH_VEC3_H

#include <cmath>
#include <cstddef>
#include <type_traits>

#include <trivial/core/assert.h>
#include <trivial/core/math/concepts.h>

namespace trivial::math {

template <Arithmetic T>
struct Vec3 {
	T x{};
	T y{};
	T z{};

	[[nodiscard]] constexpr T& operator[](std::size_t index) noexcept {
		TRIVIAL_ASSERT(index < 3);

		switch (index) {
			default:
			case 0:
				return x;
			case 1:
				return y;
			case 2:
				return z;
		}
	}

	[[nodiscard]] constexpr const T& operator[](std::size_t index) const noexcept {
		TRIVIAL_ASSERT(index < 3);

		switch (index) {
			default:
			case 0:
				return x;
			case 1:
				return y;
			case 2:
				return z;
		}
	}

	[[nodiscard]] constexpr Vec3 operator+() const noexcept { return *this; }
	[[nodiscard]] constexpr Vec3 operator-() const noexcept { return {-x, -y, -z}; }

	[[nodiscard]] constexpr Vec3 operator+(Vec3 rhs) const noexcept { return {x + rhs.x, y + rhs.y, z + rhs.z}; }
	[[nodiscard]] constexpr Vec3 operator-(Vec3 rhs) const noexcept { return {x - rhs.x, y - rhs.y, z - rhs.z}; }
	[[nodiscard]] constexpr Vec3 operator*(Vec3 rhs) const noexcept { return {x * rhs.x, y * rhs.y, z * rhs.z}; }
	[[nodiscard]] constexpr Vec3 operator/(Vec3 rhs) const noexcept { return {x / rhs.x, y / rhs.y, z / rhs.z}; }

	[[nodiscard]] constexpr Vec3 operator*(T scalar) const noexcept { return {x * scalar, y * scalar, z * scalar}; }
	[[nodiscard]] constexpr Vec3 operator/(T scalar) const noexcept { return {x / scalar, y / scalar, z / scalar}; }

	constexpr Vec3& operator+=(Vec3 rhs) noexcept {
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
		return *this;
	}

	constexpr Vec3& operator-=(Vec3 rhs) noexcept {
		x -= rhs.x;
		y -= rhs.y;
		z -= rhs.z;
		return *this;
	}

	constexpr Vec3& operator*=(Vec3 rhs) noexcept {
		x *= rhs.x;
		y *= rhs.y;
		z *= rhs.z;
		return *this;
	}

	constexpr Vec3& operator/=(Vec3 rhs) noexcept {
		x /= rhs.x;
		y /= rhs.y;
		z /= rhs.z;
		return *this;
	}

	constexpr Vec3& operator*=(T scalar) noexcept {
		x *= scalar;
		y *= scalar;
		z *= scalar;
		return *this;
	}

	constexpr Vec3& operator/=(T scalar) noexcept {
		x /= scalar;
		y /= scalar;
		z /= scalar;
		return *this;
	}

	[[nodiscard]] constexpr T dot(Vec3 rhs) const noexcept { return (x * rhs.x) + (y * rhs.y) + (z * rhs.z); }

	[[nodiscard]] constexpr Vec3 cross(Vec3 rhs) const noexcept {
		return {(y * rhs.z) - (z * rhs.y), (z * rhs.x) - (x * rhs.z), (x * rhs.y) - (y * rhs.x)};
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
		return std::hypot(x, y, z);
	}

	[[nodiscard]] Vec3 normalised() const noexcept
	    requires FloatingPoint<T>
	{
		T vectorLength = length();

		TRIVIAL_ASSERT(vectorLength != T{});

		return *this / vectorLength;
	}

	[[nodiscard]] Vec3 normalisedOrZero() const noexcept
	    requires FloatingPoint<T>
	{
		T vectorLength = length();

		if (vectorLength == T{}) {
			return {};
		}

		return *this / vectorLength;
	}

	// TODO: perp when I setle on a convention for direction

	[[nodiscard]] constexpr Vec3 lerp(Vec3 rhs, T t) const noexcept
	    requires FloatingPoint<T>
	{
		return *this + ((rhs - *this) * t);
	}

	[[nodiscard]] constexpr bool operator==(const Vec3&) const noexcept = default;

	[[nodiscard]] constexpr bool nearlyEqual(Vec3 rhs, T epsilon) const noexcept
	    requires FloatingPoint<T>
	{
		T dx = x > rhs.x ? x - rhs.x : rhs.x - x;
		T dy = y > rhs.y ? y - rhs.y : rhs.y - y;
		T dz = z > rhs.z ? z - rhs.z : rhs.z - z;

		return dx <= epsilon && dy <= epsilon && dz <= epsilon;
	}
};

template <Arithmetic T>
[[nodiscard]] constexpr Vec3<T> operator*(T scalar, Vec3<T> vector) noexcept {
	return vector * scalar;
}

template <Arithmetic T>
[[nodiscard]] constexpr T dot(Vec3<T> lhs, Vec3<T> rhs) noexcept {
	return lhs.dot(rhs);
}

template <Arithmetic T>
[[nodiscard]] constexpr Vec3<T> cross(Vec3<T> lhs, Vec3<T> rhs) noexcept {
	return lhs.cross(rhs);
}

template <FloatingPoint T>
[[nodiscard]] constexpr Vec3<T> lerp(Vec3<T> lhs, Vec3<T> rhs, T t) noexcept {
	return lhs.lerp(rhs, t);
}

template <FloatingPoint T>
[[nodiscard]] constexpr bool nearlyEqual(Vec3<T> lhs, Vec3<T> rhs, T epsilon) noexcept {
	return lhs.nearlyEqual(rhs, epsilon);
}

using Vec3i = Vec3<int>;
using Vec3f = Vec3<float>;
using Vec3d = Vec3<double>;

static_assert(sizeof(Vec3i) == sizeof(int) * 3);
static_assert(sizeof(Vec3f) == sizeof(float) * 3);
static_assert(sizeof(Vec3d) == sizeof(double) * 3);

static_assert(alignof(Vec3i) == alignof(int));
static_assert(alignof(Vec3f) == alignof(float));
static_assert(alignof(Vec3d) == alignof(double));

static_assert(std::is_trivially_copyable_v<Vec3i>);
static_assert(std::is_trivially_copyable_v<Vec3f>);
static_assert(std::is_trivially_copyable_v<Vec3d>);

static_assert(std::is_standard_layout_v<Vec3i>);
static_assert(std::is_standard_layout_v<Vec3f>);
static_assert(std::is_standard_layout_v<Vec3d>);

} // namespace trivial::math

#endif // TRIVIAL_CORE_MATH_VEC3_H

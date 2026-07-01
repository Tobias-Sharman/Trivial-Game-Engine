#ifndef TRIVIAL_CORE_MATH_VEC3_H
#define TRIVIAL_CORE_MATH_VEC3_H

#include <cmath>
#include <type_traits>

#include <trivial/core/math/concepts.h>

namespace trivial::math {

template <Arithmetic T>
inline constexpr auto g_kVec3Size = sizeof(T) * 3;

template <Arithmetic T>
struct Vec3 {
	T x{};
	T y{};
	T z{};

	constexpr Vec3() noexcept = default;

	constexpr Vec3(T x, T y, T z) noexcept
	    : x(x)
	    , y(y)
	    , z(z) {}

	[[nodiscard]] constexpr T& operator[](int index) noexcept {
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

	[[nodiscard]] constexpr const T& operator[](int index) const noexcept {
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

	[[nodiscard]] constexpr bool operator==(const Vec3& rhs) const noexcept = default;
};

template <Arithmetic T>
[[nodiscard]] constexpr Vec3<T> operator*(T scalar, Vec3<T> v) noexcept {
	return v * scalar;
}

template <Arithmetic T>
[[nodiscard]] constexpr T dot(Vec3<T> a, Vec3<T> b) noexcept {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

template <Arithmetic T>
[[nodiscard]] constexpr Vec3<T> cross(Vec3<T> a, Vec3<T> b) noexcept {
	return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

template <Arithmetic T>
[[nodiscard]] constexpr T lengthSquared(Vec3<T> v) noexcept {
	return dot(v, v);
}

template <std::floating_point T>
[[nodiscard]] T length(Vec3<T> v) noexcept {
	return std::sqrt(lengthSquared(v));
}

template <std::floating_point T>
[[nodiscard]] T robustLength(Vec3<T> v) noexcept {
	return std::hypot(v.x, v.y, v.z);
}

template <std::floating_point T>
[[nodiscard]] Vec3<T> normalise(Vec3<T> v) noexcept {
	return v / length(v);
}

template <std::floating_point T>
[[nodiscard]] Vec3<T> normaliseOrZero(Vec3<T> v) noexcept {
	T len = length(v);
	return len != T(0) ? v / len : Vec3<T>{};
}

template <std::floating_point T>
[[nodiscard]] constexpr Vec3<T> lerp(Vec3<T> a, Vec3<T> b, T t) noexcept {
	return a + (b - a) * t;
}

template <std::floating_point T>
[[nodiscard]] constexpr bool nearlyEqual(Vec3<T> a, Vec3<T> b, T epsilon) noexcept {
	T dx = a.x > b.x ? a.x - b.x : b.x - a.x;
	T dy = a.y > b.y ? a.y - b.y : b.y - a.y;
	T dz = a.z > b.z ? a.z - b.z : b.z - a.z;
	return dx <= epsilon && dy <= epsilon && dz <= epsilon;
}

using Vec3f = Vec3<float>;
using Vec3d = Vec3<double>;
using Vec3i = Vec3<int>;

static_assert(sizeof(Vec3f) == g_kVec3Size<float>);
static_assert(sizeof(Vec3d) == g_kVec3Size<double>);
static_assert(sizeof(Vec3i) == g_kVec3Size<int>);

static_assert(alignof(Vec3f) == alignof(float));
static_assert(alignof(Vec3d) == alignof(double));
static_assert(alignof(Vec3i) == alignof(int));

static_assert(std::is_trivially_copyable_v<Vec3f>);
static_assert(std::is_trivially_copyable_v<Vec3d>);
static_assert(std::is_trivially_copyable_v<Vec3i>);

static_assert(std::is_standard_layout_v<Vec3f>);
static_assert(std::is_standard_layout_v<Vec3d>);
static_assert(std::is_standard_layout_v<Vec3i>);

} // namespace trivial::math

#endif // TRIVIAL_CORE_MATH_VEC3_H

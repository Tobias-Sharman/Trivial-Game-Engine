#ifndef TRIVIAL_CORE_MATH_VEC4_H
#define TRIVIAL_CORE_MATH_VEC4_H

#include <cmath>
#include <type_traits>

#include <trivial/core/math/concepts.h>

namespace trivial::math {

template <Arithmetic T>
inline constexpr auto g_kVec4Size = sizeof(T) * 4;

template <Arithmetic T>
inline constexpr auto g_kVec4Alignment = alignof(T);

template <>
inline constexpr auto g_kVec4Alignment<float> = 16;

template <Arithmetic T>
struct alignas(g_kVec4Alignment<T>) Vec4 {
	T x{};
	T y{};
	T z{};
	T w{};

	constexpr Vec4() noexcept = default;

	constexpr Vec4(T x, T y, T z, T w) noexcept
	    : x(x)
	    , y(y)
	    , z(z)
	    , w(w) {}

	[[nodiscard]] constexpr T& operator[](int index) noexcept {
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

	[[nodiscard]] constexpr const T& operator[](int index) const noexcept {
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

	[[nodiscard]] constexpr bool operator==(const Vec4& rhs) const noexcept = default;
};

template <Arithmetic T>
[[nodiscard]] constexpr Vec4<T> operator*(T scalar, Vec4<T> v) noexcept {
	return v * scalar;
}

template <Arithmetic T>
[[nodiscard]] constexpr T dot(Vec4<T> a, Vec4<T> b) noexcept {
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

template <Arithmetic T>
[[nodiscard]] constexpr T lengthSquared(Vec4<T> v) noexcept {
	return dot(v, v);
}

template <std::floating_point T>
[[nodiscard]] T length(Vec4<T> v) noexcept {
	return std::sqrt(lengthSquared(v));
}

template <std::floating_point T>
[[nodiscard]] T robustLength(Vec4<T> v) noexcept {
	return std::hypot(v.x, v.y, v.z, v.w);
}

template <std::floating_point T>
[[nodiscard]] Vec4<T> normalise(Vec4<T> v) noexcept {
	return v / length(v);
}

template <std::floating_point T>
[[nodiscard]] Vec4<T> normaliseOrZero(Vec4<T> v) noexcept {
	T len = length(v);
	return len != T(0) ? v / len : Vec4<T>{};
}

template <std::floating_point T>
[[nodiscard]] constexpr Vec4<T> lerp(Vec4<T> a, Vec4<T> b, T t) noexcept {
	return a + (b - a) * t;
}

template <std::floating_point T>
[[nodiscard]] constexpr bool nearlyEqual(Vec4<T> a, Vec4<T> b, T epsilon) noexcept {
	T dx = a.x > b.x ? a.x - b.x : b.x - a.x;
	T dy = a.y > b.y ? a.y - b.y : b.y - a.y;
	T dz = a.z > b.z ? a.z - b.z : b.z - a.z;
	T dw = a.w > b.w ? a.w - b.w : b.w - a.w;

	return dx <= epsilon && dy <= epsilon && dz <= epsilon && dw <= epsilon;
}

using Vec4f = Vec4<float>;
using Vec4d = Vec4<double>;
using Vec4i = Vec4<int>;

static_assert(sizeof(Vec4f) == g_kVec4Size<float>);
static_assert(sizeof(Vec4d) == g_kVec4Size<double>);
static_assert(sizeof(Vec4i) == g_kVec4Size<int>);

static_assert(alignof(Vec4f) == g_kVec4Alignment<float>);
static_assert(alignof(Vec4d) == g_kVec4Alignment<double>);
static_assert(alignof(Vec4i) == g_kVec4Alignment<int>);

static_assert(std::is_trivially_copyable_v<Vec4f>);
static_assert(std::is_trivially_copyable_v<Vec4d>);
static_assert(std::is_trivially_copyable_v<Vec4i>);

static_assert(std::is_standard_layout_v<Vec4f>);
static_assert(std::is_standard_layout_v<Vec4d>);
static_assert(std::is_standard_layout_v<Vec4i>);

} // namespace trivial::math

#endif // TRIVIAL_CORE_MATH_VEC4_H

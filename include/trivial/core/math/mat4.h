#ifndef TRIVIAL_CORE_MATH_MAT4_H
#define TRIVIAL_CORE_MATH_MAT4_H

#include <trivial/core/math/concepts.h>
#include <trivial/core/math/vec3.h>
#include <trivial/core/math/vec4.h>

namespace trivial::math {

template <Arithmetic T>
inline constexpr auto g_kMat4Size = sizeof(T) * 16;

template <Arithmetic T>
struct Mat4 {
	Vec4<T> col0{};
	Vec4<T> col1{};
	Vec4<T> col2{};
	Vec4<T> col3{};

	constexpr Mat4() noexcept = default;

	constexpr Mat4(Vec4<T> col0, Vec4<T> col1, Vec4<T> col2, Vec4<T> col3) noexcept
	    : col0(col0)
	    , col1(col1)
	    , col2(col2)
	    , col3(col3) {}

	constexpr Mat4(T m00,
	               T m01,
	               T m02,
	               T m03,
	               T m10,
	               T m11,
	               T m12,
	               T m13,
	               T m20,
	               T m21,
	               T m22,
	               T m23,
	               T m30,
	               T m31,
	               T m32,
	               T m33) noexcept
	    : col0{m00, m10, m20, m30}
	    , col1{m01, m11, m21, m31}
	    , col2{m02, m12, m22, m32}
	    , col3{m03, m13, m23, m33} {}

	[[nodiscard]] constexpr Vec4<T>& operator[](int index) noexcept {
		switch (index) {
			default:
			case 0:
				return col0;
			case 1:
				return col1;
			case 2:
				return col2;
			case 3:
				return col3;
		}
	}

	[[nodiscard]] constexpr const Vec4<T>& operator[](int index) const noexcept {
		switch (index) {
			default:
			case 0:
				return col0;
			case 1:
				return col1;
			case 2:
				return col2;
			case 3:
				return col3;
		}
	}

	[[nodiscard]] static constexpr Mat4 identity() noexcept {
		return {T(1), T(0), T(0), T(0), T(0), T(1), T(0), T(0), T(0), T(0), T(1), T(0), T(0), T(0), T(0), T(1)};
	}

	[[nodiscard]] static constexpr Mat4 translation(Vec3<T> v) noexcept {
		return {T(1), T(0), T(0), v.x, T(0), T(1), T(0), v.y, T(0), T(0), T(1), v.z, T(0), T(0), T(0), T(1)};
	}

	[[nodiscard]] static constexpr Mat4 scale(Vec3<T> v) noexcept {
		return {v.x, T(0), T(0), T(0), T(0), v.y, T(0), T(0), T(0), T(0), v.z, T(0), T(0), T(0), T(0), T(1)};
	}

	[[nodiscard]] static Mat4 rotationX(T radians) noexcept
	    requires std::floating_point<T>
	{
		T cosAngle = std::cos(radians);
		T sinAngle = std::sin(radians);

		return {T(1),
		        T(0),
		        T(0),
		        T(0),
		        T(0),
		        cosAngle,
		        -sinAngle,
		        T(0),
		        T(0),
		        sinAngle,
		        cosAngle,
		        T(0),
		        T(0),
		        T(0),
		        T(0),
		        T(1)};
	}

	[[nodiscard]] static Mat4 rotationY(T radians) noexcept
	    requires std::floating_point<T>
	{
		T cosAngle = std::cos(radians);
		T sinAngle = std::sin(radians);

		return {cosAngle,
		        T(0),
		        sinAngle,
		        T(0),
		        T(0),
		        T(1),
		        T(0),
		        T(0),
		        -sinAngle,
		        T(0),
		        cosAngle,
		        T(0),
		        T(0),
		        T(0),
		        T(0),
		        T(1)};
	}

	[[nodiscard]] static Mat4 rotationZ(T radians) noexcept
	    requires std::floating_point<T>
	{
		T cosAngle = std::cos(radians);
		T sinAngle = std::sin(radians);

		return {cosAngle,
		        -sinAngle,
		        T(0),
		        T(0),
		        sinAngle,
		        cosAngle,
		        T(0),
		        T(0),
		        T(0),
		        T(0),
		        T(1),
		        T(0),
		        T(0),
		        T(0),
		        T(0),
		        T(1)};
	}

	[[nodiscard]] constexpr bool operator==(const Mat4& rhs) const noexcept = default;
};

template <Arithmetic T>
[[nodiscard]] constexpr Vec4<T> operator*(Mat4<T> m, Vec4<T> v) noexcept {
	return m.col0 * v.x + m.col1 * v.y + m.col2 * v.z + m.col3 * v.w;
}

template <Arithmetic T>
[[nodiscard]] constexpr Mat4<T> operator*(Mat4<T> lhs, Mat4<T> rhs) noexcept {
	return {lhs * rhs.col0, lhs * rhs.col1, lhs * rhs.col2, lhs * rhs.col3};
}

template <Arithmetic T>
constexpr Mat4<T>& operator*=(Mat4<T>& lhs, Mat4<T> rhs) noexcept {
	lhs = lhs * rhs;
	return lhs;
}

template <Arithmetic T>
[[nodiscard]] constexpr Mat4<T> transpose(Mat4<T> m) noexcept {
	return {m.col0.x,
	        m.col0.y,
	        m.col0.z,
	        m.col0.w,
	        m.col1.x,
	        m.col1.y,
	        m.col1.z,
	        m.col1.w,
	        m.col2.x,
	        m.col2.y,
	        m.col2.z,
	        m.col2.w,
	        m.col3.x,
	        m.col3.y,
	        m.col3.z,
	        m.col3.w};
}

template <std::floating_point T>
[[nodiscard]] constexpr bool nearlyEqual(Mat4<T> lhs, Mat4<T> rhs, T epsilon) noexcept {
	return nearlyEqual(lhs.col0, rhs.col0, epsilon) && nearlyEqual(lhs.col1, rhs.col1, epsilon)
	       && nearlyEqual(lhs.col2, rhs.col2, epsilon) && nearlyEqual(lhs.col3, rhs.col3, epsilon);
}

using Mat4f = Mat4<float>;
using Mat4d = Mat4<double>;
using Mat4i = Mat4<int>;

static_assert(sizeof(Mat4f) == g_kMat4Size<float>);
static_assert(sizeof(Mat4d) == g_kMat4Size<double>);
static_assert(sizeof(Mat4i) == g_kMat4Size<int>);

static_assert(alignof(Mat4f) == alignof(Vec4f));
static_assert(alignof(Mat4d) == alignof(Vec4d));
static_assert(alignof(Mat4i) == alignof(Vec4i));

static_assert(std::is_trivially_copyable_v<Mat4f>);
static_assert(std::is_trivially_copyable_v<Mat4d>);
static_assert(std::is_trivially_copyable_v<Mat4i>);

static_assert(std::is_standard_layout_v<Mat4f>);
static_assert(std::is_standard_layout_v<Mat4d>);
static_assert(std::is_standard_layout_v<Mat4i>);

} // namespace trivial::math

#endif // TRIVIAL_CORE_MATH_MAT4_H

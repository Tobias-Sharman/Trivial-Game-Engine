#ifndef TRIVIAL_CORE_MATH_MAT4_H
#define TRIVIAL_CORE_MATH_MAT4_H

#include <cmath>
#include <cstddef>
#include <type_traits>

#include <trivial/core/assert.h>
#include <trivial/core/math/angle.h>
#include <trivial/core/math/concepts.h>
#include <trivial/core/math/vec3.h>
#include <trivial/core/math/vec4.h>

namespace trivial::math {

template <FloatingPoint T>
struct Mat4 {
	Vec4<T> col0{};
	Vec4<T> col1{};
	Vec4<T> col2{};
	Vec4<T> col3{};

	[[nodiscard]] constexpr Vec4<T>& operator[](std::size_t index) noexcept {
		TRIVIAL_ASSERT(index < 4);

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

	[[nodiscard]] constexpr const Vec4<T>& operator[](std::size_t index) const noexcept {
		TRIVIAL_ASSERT(index < 4);

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
		return {{T{1}, T{0}, T{0}, T{0}}, {T{0}, T{1}, T{0}, T{0}}, {T{0}, T{0}, T{1}, T{0}}, {T{0}, T{0}, T{0}, T{1}}};
	}

	[[nodiscard]] static constexpr Mat4 translation(const Vec3<T>& translation) noexcept {
		return {{T{1}, T{0}, T{0}, T{0}},
		        {T{0}, T{1}, T{0}, T{0}},
		        {T{0}, T{0}, T{1}, T{0}},
		        {translation.x, translation.y, translation.z, T{1}}};
	}

	[[nodiscard]] static constexpr Mat4 scale(const Vec3<T>& scale) noexcept {
		return {{scale.x, T{0}, T{0}, T{0}},
		        {T{0}, scale.y, T{0}, T{0}},
		        {T{0}, T{0}, scale.z, T{0}},
		        {T{0}, T{0}, T{0}, T{1}}};
	}

	[[nodiscard]] static Mat4 rotationX(Angle<T> angle) noexcept {
		const T kCosAngle = std::cos(angle.radians());
		const T kSinAngle = std::sin(angle.radians());

		return {{T{1}, T{0}, T{0}, T{0}},
		        {T{0}, kCosAngle, kSinAngle, T{0}},
		        {T{0}, -kSinAngle, kCosAngle, T{0}},
		        {T{0}, T{0}, T{0}, T{1}}};
	}

	[[nodiscard]] static Mat4 rotationY(Angle<T> angle) noexcept {
		const T kCosAngle = std::cos(angle.radians());
		const T kSinAngle = std::sin(angle.radians());

		return {{kCosAngle, T{0}, -kSinAngle, T{0}},
		        {T{0}, T{1}, T{0}, T{0}},
		        {kSinAngle, T{0}, kCosAngle, T{0}},
		        {T{0}, T{0}, T{0}, T{1}}};
	}

	[[nodiscard]] static Mat4 rotationZ(Angle<T> angle) noexcept {
		const T kCosAngle = std::cos(angle.radians());
		const T kSinAngle = std::sin(angle.radians());

		return {{kCosAngle, kSinAngle, T{0}, T{0}},
		        {-kSinAngle, kCosAngle, T{0}, T{0}},
		        {T{0}, T{0}, T{1}, T{0}},
		        {T{0}, T{0}, T{0}, T{1}}};
	}

	[[nodiscard]] constexpr bool operator==(const Mat4& rhs) const noexcept = default;

	[[nodiscard]] constexpr bool nearlyEqual(const Mat4& rhs, T epsilon) const noexcept {
		return trivial::math::nearlyEqual(col0, rhs.col0, epsilon)
		       && trivial::math::nearlyEqual(col1, rhs.col1, epsilon)
		       && trivial::math::nearlyEqual(col2, rhs.col2, epsilon)
		       && trivial::math::nearlyEqual(col3, rhs.col3, epsilon);
	}
};

using Mat4f = Mat4<float>;
using Mat4d = Mat4<double>;

template <FloatingPoint T>
[[nodiscard]] constexpr Mat4<T> fromRows(T m00,
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
                                         T m33) noexcept {
	return {{m00, m10, m20, m30}, {m01, m11, m21, m31}, {m02, m12, m22, m32}, {m03, m13, m23, m33}};
}
namespace detail {

[[nodiscard]] Vec4f multiplyMat4Vec4(const Mat4f& matrix, const Vec4f& vector) noexcept;

[[nodiscard]] Mat4f multiplyMat4Mat4(const Mat4f& lhs, const Mat4f& rhs) noexcept;

template <FloatingPoint T>
[[nodiscard]] constexpr Vec4<T> multiplyMat4Vec4Scalar(const Mat4<T>& matrix, const Vec4<T>& vector) noexcept {
	return (matrix.col0 * vector.x + matrix.col1 * vector.y) + (matrix.col2 * vector.z + matrix.col3 * vector.w);
}

template <FloatingPoint T>
[[nodiscard]] constexpr Mat4<T> multiplyMat4Mat4Scalar(const Mat4<T>& lhs, const Mat4<T>& rhs) noexcept {
	return {(lhs.col0 * rhs.col0.x + lhs.col1 * rhs.col0.y) + (lhs.col2 * rhs.col0.z + lhs.col3 * rhs.col0.w),
	        (lhs.col0 * rhs.col1.x + lhs.col1 * rhs.col1.y) + (lhs.col2 * rhs.col1.z + lhs.col3 * rhs.col1.w),
	        (lhs.col0 * rhs.col2.x + lhs.col1 * rhs.col2.y) + (lhs.col2 * rhs.col2.z + lhs.col3 * rhs.col2.w),
	        (lhs.col0 * rhs.col3.x + lhs.col1 * rhs.col3.y) + (lhs.col2 * rhs.col3.z + lhs.col3 * rhs.col3.w)};
}

} // namespace detail

template <FloatingPoint T>
[[nodiscard]] constexpr Vec4<T> operator*(const Mat4<T>& matrix, const Vec4<T>& vector) noexcept {
	if constexpr (std::is_same_v<T, float>) {
		if (!std::is_constant_evaluated()) {
			return detail::multiplyMat4Vec4(matrix, vector);
		}
	}
	return detail::multiplyMat4Vec4Scalar(matrix, vector);
}

template <FloatingPoint T>
[[nodiscard]] constexpr Mat4<T> operator*(const Mat4<T>& lhs, const Mat4<T>& rhs) noexcept {
	if constexpr (std::is_same_v<T, float>) {
		if (!std::is_constant_evaluated()) {
			return detail::multiplyMat4Mat4(lhs, rhs);
		}
	}
	return detail::multiplyMat4Mat4Scalar(lhs, rhs);
}

template <FloatingPoint T>
[[nodiscard]] constexpr Mat4<T> transpose(const Mat4<T>& matrix) noexcept {
	return {{matrix.col0.x, matrix.col1.x, matrix.col2.x, matrix.col3.x},
	        {matrix.col0.y, matrix.col1.y, matrix.col2.y, matrix.col3.y},
	        {matrix.col0.z, matrix.col1.z, matrix.col2.z, matrix.col3.z},
	        {matrix.col0.w, matrix.col1.w, matrix.col2.w, matrix.col3.w}};
}
template <std::floating_point T>
[[nodiscard]] constexpr bool nearlyEqual(const Mat4<T>& lhs, const Mat4<T>& rhs, T epsilon) noexcept {
	return lhs.nearlyEqual(rhs, epsilon);
}

using Mat4f = Mat4<float>;
using Mat4d = Mat4<double>;

static_assert(sizeof(Mat4f) == sizeof(float) * 16);
static_assert(sizeof(Mat4d) == sizeof(double) * 16);

static_assert(alignof(Mat4f) == alignof(Vec4f));
static_assert(alignof(Mat4d) == alignof(Vec4d));

static_assert(std::is_trivially_copyable_v<Mat4f>);
static_assert(std::is_trivially_copyable_v<Mat4d>);

static_assert(std::is_standard_layout_v<Mat4f>);
static_assert(std::is_standard_layout_v<Mat4d>);

} // namespace trivial::math

#endif // TRIVIAL_CORE_MATH_MAT4_H

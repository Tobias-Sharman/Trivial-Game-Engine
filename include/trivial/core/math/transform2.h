#ifndef TRIVIAL_CORE_MATH_TRANSFORM2_H
#define TRIVIAL_CORE_MATH_TRANSFORM2_H

#include <type_traits>

#include <trivial/core/math/affine2.h>
#include <trivial/core/math/angle.h>
#include <trivial/core/math/concepts.h>
#include <trivial/core/math/vec2.h>

namespace trivial::math {

template <FloatingPoint T>
struct Transform2 {
	Vec2<T> position{};
	Angle<T> rotation{};
	Vec2<T> scale{T{1}, T{1}};

	[[nodiscard]] static constexpr Transform2 identity() noexcept { return {}; }

	[[nodiscard]] Affine2<T> affine() const noexcept {
		Affine2<T> translation = Affine2<T>::translation(position);
		Affine2<T> rotationTransform = Affine2<T>::rotation(rotation);
		Affine2<T> scaleTransform = Affine2<T>::scale(scale);

		return translation * rotationTransform * scaleTransform;
	}

	[[nodiscard]] constexpr bool operator==(const Transform2& rhs) const noexcept = default;

	[[nodiscard]] constexpr bool nearlyEqual(const Transform2& rhs, T epsilon) const noexcept {
		return position.nearlyEqual(rhs.position, epsilon) && rotation.nearlyEqual(rhs.rotation, epsilon)
		       && scale.nearlyEqual(rhs.scale, epsilon);
	}
};

template <FloatingPoint T>
[[nodiscard]] Affine2<T> toAffine(const Transform2<T>& transform) noexcept {
	return transform.affine();
}

template <FloatingPoint T>
[[nodiscard]] constexpr bool nearlyEqual(const Transform2<T>& lhs, const Transform2<T>& rhs, T epsilon) noexcept {
	return lhs.nearlyEqual(rhs, epsilon);
}

using Transform2f = Transform2<float>;
using Transform2d = Transform2<double>;

static_assert(sizeof(Transform2f) == sizeof(Vec2f) + sizeof(Anglef) + sizeof(Vec2f));
static_assert(sizeof(Transform2d) == sizeof(Vec2d) + sizeof(Angled) + sizeof(Vec2d));

static_assert(alignof(Transform2f) == alignof(float));
static_assert(alignof(Transform2d) == alignof(double));

static_assert(std::is_trivially_copyable_v<Transform2f>);
static_assert(std::is_trivially_copyable_v<Transform2d>);

static_assert(std::is_standard_layout_v<Transform2f>);
static_assert(std::is_standard_layout_v<Transform2d>);

} // namespace trivial::math

#endif // TRIVIAL_CORE_MATH_TRANSFORM2_H

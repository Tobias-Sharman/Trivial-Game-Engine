#ifndef TRIVIAL_CORE_MATH_TRANSFORM2D_H
#define TRIVIAL_CORE_MATH_TRANSFORM2D_H

#include <trivial/core/math/mat2x3.h>
#include <trivial/core/math/vec2.h>

namespace trivial::math {

inline constexpr auto g_kTransform2DSize = sizeof(Vec2f) + sizeof(float) + sizeof(Vec2f);

struct Transform2D {
	Vec2f position;
	float rotation{};
	Vec2f scale{1.0f, 1.0f};

	constexpr Transform2D() noexcept = default;

	constexpr Transform2D(Vec2f position, float rotation, Vec2f scale) noexcept
	    : position(position)
	    , rotation(rotation)
	    , scale(scale) {}

	[[nodiscard]] static constexpr Transform2D identity() noexcept { return {}; }

	[[nodiscard]] Mat2x3f matrix() const noexcept {
		float cosAngle = std::cos(rotation);
		float sinAngle = std::sin(rotation);

		return {cosAngle * scale.x,
		        -sinAngle * scale.y,
		        position.x,

		        sinAngle * scale.x,
		        cosAngle * scale.y,
		        position.y};
	}
};

[[nodiscard]] inline Mat2x3f toMatrix(const Transform2D& transform) noexcept {
	return transform.matrix();
}

static_assert(sizeof(Transform2D) == g_kTransform2DSize);
static_assert(alignof(Transform2D) == alignof(float));

static_assert(std::is_trivially_copyable_v<Transform2D>);
static_assert(std::is_standard_layout_v<Transform2D>);

} // namespace trivial::math

#endif // TRIVIAL_CORE_MATH_TRANSFORM2D_H

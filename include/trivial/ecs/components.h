#ifndef TRIVIAL_ECS_COMPONENTS_H
#define TRIVIAL_ECS_COMPONENTS_H

#include <trivial/core/math/vec2.h>

namespace trivial::ecs {

struct Position2D {
	trivial::math::Vec2f value;
};

struct Velocity2D {
	trivial::math::Vec2f value;
};

} // namespace trivial::ecs

#endif // TRIVIAL_ECS_COMPONENTS_H

#ifndef TRIVIAL_RHI_MESH_TYPES_H
#define TRIVIAL_RHI_MESH_TYPES_H

#include <array>
#include <cstdint>

#include <trivial/core/math/vec2.h>

namespace trivial::rhi {

struct Vertex2 {
	math::Vec2f position{};
	std::array<std::uint8_t, 4> colour{255, 255, 255, 255}; // NOLINT(readability-magic-numbers)
};

using MeshHandle = std::uint32_t;

} // namespace trivial::rhi

#endif // TRIVIAL_RHI_MESH_TYPES_H

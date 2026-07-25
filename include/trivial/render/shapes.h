#ifndef TRIVIAL_RENDER_SHAPES_H
#define TRIVIAL_RENDER_SHAPES_H

#include <cstdint>
#include <vector>

#include <trivial/rhi/mesh_types.h>

namespace trivial::render {

struct MeshGeometry {
	std::vector<rhi::Vertex2> vertices;
	std::vector<std::uint16_t> indices;
};

[[nodiscard]] MeshGeometry makeQuad() noexcept;
[[nodiscard]] MeshGeometry makeCircle(std::uint32_t segments) noexcept;

} // namespace trivial::render

#endif // TRIVIAL_RENDER_SHAPES_H

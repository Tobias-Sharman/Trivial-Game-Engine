#include <trivial/render/shapes.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>

#include <trivial/core/assert.h>

#if TRIVIAL_ENABLE_ASSERTS
#include <limits>
#endif // TRIVIAL_ENABLE_ASSERTS

// TODO: Deside on sensible defaults

namespace trivial::render {

MeshGeometry makeQuad() noexcept {
	MeshGeometry geometry;

	geometry.vertices = {
	    {.position = {.x = -0.5F, .y = -0.5F}}, // NOLINT(readability-magic-numbers)
	    {.position = {.x = 0.5F, .y = -0.5F}},  // NOLINT(readability-magic-numbers)
	    {.position = {.x = 0.5F, .y = 0.5F}},   // NOLINT(readability-magic-numbers)
	    {.position = {.x = -0.5F, .y = 0.5F}},  // NOLINT(readability-magic-numbers)
	};

	geometry.indices = {0, 1, 2, 2, 3, 0};

	return geometry;
}

MeshGeometry makeCircle(std::uint32_t segments) noexcept {
	TRIVIAL_ASSERT(segments >= 3);
	TRIVIAL_ASSERT(segments <= std::numeric_limits<std::uint16_t>::max());

	MeshGeometry geometry;
	geometry.vertices.reserve(segments + 1);

	geometry.vertices.push_back({.position = {.x = 0.0F, .y = 0.0F}}); // centre vertex

	for (std::uint32_t i = 0; i < segments; ++i) {
		const float kTheta = (2.0F * std::numbers::pi_v<float> * static_cast<float>(i)) / static_cast<float>(segments);

		// NOLINTNEXTLINE(readability-magic-numbers)
		geometry.vertices.push_back({.position = {.x = 0.5F * std::cos(kTheta), .y = 0.5F * std::sin(kTheta)}});
	}

	geometry.indices.reserve(static_cast<std::size_t>(segments) * 3U);

	for (std::uint32_t i = 0; i < segments; ++i) {
		const std::uint16_t kCurrent = static_cast<std::uint16_t>(1 + i);
		const std::uint16_t kNext = static_cast<std::uint16_t>(1 + ((i + 1) % segments));

		geometry.indices.push_back(0);
		geometry.indices.push_back(kCurrent);
		geometry.indices.push_back(kNext);
	}

	return geometry;
}

} // namespace trivial::render

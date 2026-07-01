#ifndef TRIVIAL_CORE_GRAPHICS_API_H
#define TRIVIAL_CORE_GRAPHICS_API_H

#include <cstdint>

namespace trivial {

enum class GraphicsApi : std::uint8_t {
	Auto,
	Vulkan
};

} // namespace trivial

#endif // TRIVIAL_CORE_GRAPHICS_API_H

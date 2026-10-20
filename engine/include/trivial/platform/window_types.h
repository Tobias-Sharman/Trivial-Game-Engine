#ifndef TRIVIAL_PLATFORM_WINDOW_TYPES_H
#define TRIVIAL_PLATFORM_WINDOW_TYPES_H

#include <cstdint>
#include <string>

#include <trivial/platform/platform_config.h>

namespace trivial::platform {

struct WindowSize {
	std::uint32_t width = TRIVIAL_PLATFORM_DEFAULT_WINDOW_WIDTH;
	std::uint32_t height = TRIVIAL_PLATFORM_DEFAULT_WINDOW_HEIGHT;
};

struct WindowConfig {
	WindowSize size = {};
	std::string title = TRIVIAL_PLATFORM_DEFAULT_WINDOW_TITLE;
};

} // namespace trivial::platform

#endif // TRIVIAL_PLATFORM_WINDOW_TYPES_H

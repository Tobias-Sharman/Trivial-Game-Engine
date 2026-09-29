#ifndef TRIVIAL_PLATFORM_WINDOW_BACKEND_H
#define TRIVIAL_PLATFORM_WINDOW_BACKEND_H

#include <trivial/core/config.h>

#ifdef TRIVIAL_PLATFORM_GLFW
#include <trivial/platform/glfw/window.h> // IWYU pragma: export
#else
#error "No Trivial platform window backend selected."
#endif

namespace trivial::platform {

#ifdef TRIVIAL_PLATFORM_GLFW
using WindowBackend = glfw::Window;
#endif

} // namespace trivial::platform

#endif // TRIVIAL_PLATFORM_WINDOW_BACKEND_H

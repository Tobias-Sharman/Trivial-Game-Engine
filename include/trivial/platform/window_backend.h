#ifndef TRIVIAL_PLATFORM_WINDOW_BACKEND_H
#define TRIVIAL_PLATFORM_WINDOW_BACKEND_H

#if defined(TRIVIAL_PLATFORM_GLFW)
#include <trivial/platform/glfw/window.h>
#else
#error "No Trivial platform window backend selected."
#endif

namespace trivial::platform {

#if defined(TRIVIAL_PLATFORM_GLFW)
using WindowBackend = glfw::Window;
#endif

} // namespace trivial::platform

#endif // TRIVIAL_PLATFORM_WINDOW_BACKEND_H

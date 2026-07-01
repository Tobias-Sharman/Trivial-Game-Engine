#ifndef TRIVIAL_INTERNAL_PLATFORM_WINDOW_BACKEND_H
#define TRIVIAL_INTERNAL_PLATFORM_WINDOW_BACKEND_H

#if defined(TRIVIAL_PLATFORM_GLFW)
#include <trivial/internal/platform/glfw/window.h>
#else
#error "No Trivial platform window backend selected."
#endif

namespace trivial::internal::platform {

#if defined(TRIVIAL_PLATFORM_GLFW)
using WindowBackend = glfw::Window;
#endif

} // namespace trivial::internal::platform

#endif // TRIVIAL_INTERNAL_PLATFORM_WINDOW_BACKEND_H

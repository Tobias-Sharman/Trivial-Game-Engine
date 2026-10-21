#ifndef TRIVIAL_SRC_CORE_TIME_TIME_CAPABILITIES_H
#define TRIVIAL_SRC_CORE_TIME_TIME_CAPABILITIES_H

#include <trivial/core/platform.h>

#if !(TRIVIAL_PLATFORM_WINDOWS || TRIVIAL_PLATFORM_MACOS)
#include <trivial/core/compiler.h>
#endif // !(TRIVIAL_PLATFORM_WINDOWS || TRIVIAL_PLATFORM_MACOS)

namespace trivial::time {

#if TRIVIAL_PLATFORM_WINDOWS || TRIVIAL_PLATFORM_MACOS
void initCapabilities() noexcept;

#else
TRIVIAL_FORCE_INLINE constexpr void initCapabilities() noexcept {
}

#endif // TRIVIAL_PLATFORM_WINDOWS || TRIVIAL_PLATFORM_MACOS

} // namespace trivial::time

#endif // TRIVIAL_SRC_CORE_TIME_TIME_CAPABILITIES_H

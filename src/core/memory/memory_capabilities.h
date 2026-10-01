#ifndef TRIVIAL_SRC_CORE_MEMORY_MEMORY_CAPABILITIES_H
#define TRIVIAL_SRC_CORE_MEMORY_MEMORY_CAPABILITIES_H

#include <cstddef>

#include <trivial/core/compiler.h>
#include <trivial/core/memory/memory_config.h>
#include <trivial/core/platform.h>

#if !TRIVIAL_PLATFORM_PAGE_SIZE_KNOWN
#include <trivial/core/assert.h>
#endif // !TRIVIAL_PLATFORM_PAGE_SIZE_KNOWN

#define TRIVIAL_MEMORY_HAS_DETECTED_CAPABILITIES                                                                       \
	(!TRIVIAL_PLATFORM_PAGE_SIZE_KNOWN || TRIVIAL_MEMORY_ENABLE_LARGE_PAGES)

#if !TRIVIAL_MEMORY_HAS_DETECTED_CAPABILITIES
#include <type_traits>
#endif // !TRIVIAL_MEMORY_HAS_DETECTED_CAPABILITIES

namespace trivial::memory {

struct DetectedCapabilities {
#if !TRIVIAL_PLATFORM_PAGE_SIZE_KNOWN
	std::size_t pageSize = 0;
	std::size_t allocationGranularity = 0;
#endif // !TRIVIAL_PLATFORM_PAGE_SIZE_KNOWN

#if TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
	std::size_t largePageSize = 0;
#endif // TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
};

#if !TRIVIAL_MEMORY_HAS_DETECTED_CAPABILITIES
static_assert(std::is_empty_v<DetectedCapabilities>,
              "A detected capability was added without updating TRIVIAL_MEMORY_HAS_DETECTED_CAPABILITIES");
#endif // !TRIVIAL_MEMORY_HAS_DETECTED_CAPABILITIES

#if TRIVIAL_MEMORY_HAS_DETECTED_CAPABILITIES
namespace detail {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
inline constinit DetectedCapabilities g_detectedCapabilities;

} // namespace detail
#endif // TRIVIAL_MEMORY_HAS_DETECTED_CAPABILITIES

void initCapabilities() noexcept;

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr std::size_t pageSize() noexcept {
#if TRIVIAL_PLATFORM_PAGE_SIZE_KNOWN
	return TRIVIAL_PLATFORM_PAGE_SIZE;
#else
	TRIVIAL_ASSERT(detail::g_detectedCapabilities.pageSize != 0);
	return detail::g_detectedCapabilities.pageSize;
#endif // TRIVIAL_PLATFORM_PAGE_SIZE_KNOWN
}

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr std::size_t allocationGranularity() noexcept {
#if TRIVIAL_PLATFORM_PAGE_SIZE_KNOWN
	return TRIVIAL_PLATFORM_ALLOCATION_GRANULARITY;
#else
	TRIVIAL_ASSERT(detail::g_detectedCapabilities.allocationGranularity != 0);
	return detail::g_detectedCapabilities.allocationGranularity;
#endif // TRIVIAL_PLATFORM_PAGE_SIZE_KNOWN
}

#if TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
[[nodiscard]] TRIVIAL_FORCE_INLINE std::size_t largePageSize() noexcept {
	return detail::g_detectedCapabilities.largePageSize;
}
#endif // TRIVIAL_MEMORY_ENABLE_LARGE_PAGES

} // namespace trivial::memory

#ifndef TRIVIAL_MEMORY_CAPABILITIES_IMPLEMENTATION
#undef TRIVIAL_MEMORY_HAS_DETECTED_CAPABILITIES
#endif // TRIVIAL_MEMORY_CAPABILITIES_IMPLEMENTATION

#endif // TRIVIAL_SRC_CORE_MEMORY_MEMORY_CAPABILITIES_H

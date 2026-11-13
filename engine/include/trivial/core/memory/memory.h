#ifndef TRIVIAL_CORE_MEMORY_MEMORY_H
#define TRIVIAL_CORE_MEMORY_MEMORY_H

#include <trivial/core/assert.h>
#include <trivial/core/compiler.h>
#include <trivial/core/memory/byte_count.h>
#include <trivial/core/platform.h>

namespace trivial::memory {

namespace detail {

struct DetectedCapabilities {
#if !TRIVIAL_PLATFORM_OS_PAGE_SIZE_KNOWN
	ByteCount osPageSize;
	ByteCount osAllocationGranularity;
#endif // !TRIVIAL_PLATFORM_OS_PAGE_SIZE_KNOWN
	ByteCount physicalMemorySize;
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
inline constinit DetectedCapabilities g_detectedCapabilities;

} // namespace detail

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr ByteCount osPageSize() noexcept {
#if TRIVIAL_PLATFORM_OS_PAGE_SIZE_KNOWN
	return bytes(TRIVIAL_PLATFORM_OS_PAGE_SIZE);
#else
	TRIVIAL_ASSERT(!detail::g_detectedCapabilities.osPageSize.isZero());
	return detail::g_detectedCapabilities.osPageSize;
#endif // TRIVIAL_PLATFORM_OS_PAGE_SIZE_KNOWN
}

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr ByteCount osAllocationGranularity() noexcept {
#if TRIVIAL_PLATFORM_OS_PAGE_SIZE_KNOWN
	return bytes(TRIVIAL_PLATFORM_OS_ALLOCATION_GRANULARITY);
#else
	TRIVIAL_ASSERT(!detail::g_detectedCapabilities.osAllocationGranularity.isZero());
	return detail::g_detectedCapabilities.osAllocationGranularity;
#endif // TRIVIAL_PLATFORM_OS_PAGE_SIZE_KNOWN
}

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr ByteCount physicalMemorySize() noexcept {
	TRIVIAL_ASSERT(!detail::g_detectedCapabilities.physicalMemorySize.isZero());
	return detail::g_detectedCapabilities.physicalMemorySize;
}

} // namespace trivial::memory

#endif // TRIVIAL_CORE_MEMORY_MEMORY_H

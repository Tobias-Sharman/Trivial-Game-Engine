#define TRIVIAL_MEMORY_CAPABILITIES_IMPLEMENTATION
#include "core/memory/memory_capabilities.h"

#include <trivial/core/config.h>

#if TRIVIAL_MEMORY_HAS_DETECTED_CAPABILITIES || TRIVIAL_ENABLE_ASSERTS
#include <trivial/core/memory/memory_config.h>
#include <trivial/core/platform.h>

#if TRIVIAL_PLATFORM_PAGE_SIZE_KNOWN || (TRIVIAL_MEMORY_LAZY_DECOMMIT && TRIVIAL_PLATFORM_LINUX)
#include <trivial/core/assert.h>
#endif // TRIVIAL_PLATFORM_PAGE_SIZE_KNOWN || (TRIVIAL_MEMORY_LAZY_DECOMMIT && TRIVIAL_PLATFORM_LINUX)

#include "core/memory/virtual_memory.h"
#endif // TRIVIAL_MEMORY_HAS_DETECTED_CAPABILITIES || TRIVIAL_ENABLE_ASSERTS

namespace trivial::memory {

void initCapabilities() noexcept {
#if TRIVIAL_MEMORY_HAS_DETECTED_CAPABILITIES || TRIVIAL_ENABLE_ASSERTS
	const SystemInfo kSystemInfo = probeSystemInfo();

#if TRIVIAL_PLATFORM_PAGE_SIZE_KNOWN
	TRIVIAL_ASSERT(kSystemInfo.pageSize == TRIVIAL_PLATFORM_PAGE_SIZE);
	TRIVIAL_ASSERT(kSystemInfo.allocationGranularity == TRIVIAL_PLATFORM_ALLOCATION_GRANULARITY);
#endif // TRIVIAL_PLATFORM_PAGE_SIZE_KNOWN

#if TRIVIAL_MEMORY_LAZY_DECOMMIT && TRIVIAL_PLATFORM_LINUX
	TRIVIAL_ASSERT(probeMadvFree(kSystemInfo.pageSize));
#endif // TRIVIAL_MEMORY_LAZY_DECOMMIT && TRIVIAL_PLATFORM_LINUX

#if TRIVIAL_MEMORY_HAS_DETECTED_CAPABILITIES
	detail::g_detectedCapabilities = DetectedCapabilities{
#if !TRIVIAL_PLATFORM_PAGE_SIZE_KNOWN
	    .pageSize = kSystemInfo.pageSize,
	    .allocationGranularity = kSystemInfo.allocationGranularity,
#endif // !TRIVIAL_PLATFORM_PAGE_SIZE_KNOWN
#if TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
	    .largePageSize = kSystemInfo.largePageSize,
#endif // TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
	};
#endif // TRIVIAL_MEMORY_HAS_DETECTED_CAPABILITIES
#endif // TRIVIAL_MEMORY_HAS_DETECTED_CAPABILITIES || TRIVIAL_ENABLE_ASSERTS
}

} // namespace trivial::memory

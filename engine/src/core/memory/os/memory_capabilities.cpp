#include "core/memory/os/memory_capabilities.h"

#include <trivial/core/config.h>
#include <trivial/core/log.h>
#include <trivial/core/memory/byte_count.h>
#include <trivial/core/memory/memory.h>
#include <trivial/core/memory/memory_config.h>
#include <trivial/core/platform.h>

#include "core/memory/os/os_memory.h"

#if TRIVIAL_PLATFORM_OS_PAGE_SIZE_KNOWN && TRIVIAL_ENABLE_ASSERTS
#include <trivial/core/assert.h>
#endif // TRIVIAL_PLATFORM_OS_PAGE_SIZE_KNOWN && TRIVIAL_ENABLE_ASSERTS

namespace trivial::memory {

void initCapabilities() noexcept {
#if !TRIVIAL_PLATFORM_OS_PAGE_SIZE_KNOWN || TRIVIAL_ENABLE_ASSERTS
	const PageInfo kPageInfo = probePageInfo();

#if TRIVIAL_PLATFORM_OS_PAGE_SIZE_KNOWN
	TRIVIAL_ASSERT(kPageInfo.osPageSize == bytes(TRIVIAL_PLATFORM_OS_PAGE_SIZE));
	TRIVIAL_ASSERT(kPageInfo.osAllocationGranularity == bytes(TRIVIAL_PLATFORM_OS_ALLOCATION_GRANULARITY));
#else
	detail::g_detectedCapabilities.osPageSize = kPageInfo.osPageSize;
	detail::g_detectedCapabilities.osAllocationGranularity = kPageInfo.osAllocationGranularity;
#endif // TRIVIAL_PLATFORM_OS_PAGE_SIZE_KNOWN

#endif // !TRIVIAL_PLATFORM_OS_PAGE_SIZE_KNOWN || TRIVIAL_ENABLE_ASSERTS

	detail::g_detectedCapabilities.physicalMemorySize = probePhysicalMemorySize();

	if (detail::g_detectedCapabilities.physicalMemorySize.isZero()) {
		TRIVIAL_LOG_ERROR_PREFIX("Memory", "physical memory size unknown, using the default");
		detail::g_detectedCapabilities.physicalMemorySize = bytes(TRIVIAL_MEMORY_DEFAULT_PHYSICAL_MEMORY_BYTES);
	}
}

} // namespace trivial::memory

#ifndef TRIVIAL_SRC_CORE_MEMORY_OS_OS_MEMORY_H
#define TRIVIAL_SRC_CORE_MEMORY_OS_OS_MEMORY_H

#include <trivial/core/memory/byte_count.h>
#include <trivial/core/memory/memory_config.h>
#include <trivial/core/platform.h>

#define TRIVIAL_MEMORY_LAZY_DECOMMIT                                                                                   \
	(TRIVIAL_MEMORY_ENABLE_DECOMMIT && TRIVIAL_MEMORY_PREFER_LAZY_DECOMMIT                                             \
	 && (TRIVIAL_PLATFORM_WINDOWS || TRIVIAL_PLATFORM_SDK_HAS_MADV_FREE))

#define TRIVIAL_MEMORY_LAZY_DECOMMIT_MAX_RETRIES 3

#define TRIVIAL_MEMORY_DECOMMIT_ZEROES                                                                                 \
	(TRIVIAL_MEMORY_ENABLE_DECOMMIT && !TRIVIAL_MEMORY_LAZY_DECOMMIT && !TRIVIAL_PLATFORM_MACOS)

namespace trivial::memory {

struct PageInfo {
	ByteCount osPageSize;
	ByteCount osAllocationGranularity;
};

[[nodiscard]] PageInfo probePageInfo() noexcept;

[[nodiscard]] ByteCount probePhysicalMemorySize() noexcept;

[[nodiscard]] void* reserveAligned(ByteCount size, ByteCount alignment, int& outOsErrorCode) noexcept;

void releaseReservation(void* base, ByteCount size) noexcept;

[[nodiscard]] bool commitPages(void* addr, ByteCount size, int& outOsErrorCode) noexcept;

#if TRIVIAL_MEMORY_ENABLE_DECOMMIT
void decommitPages(void* addr, ByteCount size) noexcept;

#if TRIVIAL_MEMORY_LAZY_DECOMMIT
void lazyDecommitPages(void* addr, ByteCount size) noexcept;
#endif // TRIVIAL_MEMORY_LAZY_DECOMMIT
#endif // TRIVIAL_MEMORY_ENABLE_DECOMMIT

[[nodiscard]] bool makePagesNoAccess(void* addr, ByteCount size, int& outOsErrorCode) noexcept;

[[nodiscard]] bool makePagesReadOnly(void* addr, ByteCount size, int& outOsErrorCode) noexcept;

[[nodiscard]] bool makePagesReadWrite(void* addr, ByteCount size, int& outOsErrorCode) noexcept;

} // namespace trivial::memory

#ifndef TRIVIAL_MEMORY_OS_MEMORY_IMPLEMENTATION
#undef TRIVIAL_MEMORY_LAZY_DECOMMIT_MAX_RETRIES
#endif // TRIVIAL_MEMORY_OS_MEMORY_IMPLEMENTATION

#endif // TRIVIAL_SRC_CORE_MEMORY_OS_OS_MEMORY_H

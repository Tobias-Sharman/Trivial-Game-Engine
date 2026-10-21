#ifndef TRIVIAL_SRC_CORE_MEMORY_VIRTUAL_MEMORY_H
#define TRIVIAL_SRC_CORE_MEMORY_VIRTUAL_MEMORY_H

#include <cstddef>

#include <trivial/core/memory/memory_config.h>
#include <trivial/core/platform.h>

namespace trivial::memory {

struct SystemInfo {
	std::size_t pageSize = 0;
	std::size_t allocationGranularity = 0;
#if TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
	std::size_t largePageSize = 0;
#endif // TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
};

#if TRIVIAL_MEMORY_ENABLE_LARGE_PAGES && TRIVIAL_PLATFORM_WINDOWS
bool adjustLockMemoryPrivilege(bool enable) noexcept;
#endif // TRIVIAL_MEMORY_ENABLE_LARGE_PAGES && TRIVIAL_PLATFORM_WINDOWS

[[nodiscard]] SystemInfo probeSystemInfo() noexcept;

#if TRIVIAL_PLATFORM_LINUX
bool probeMadvFree(std::size_t pageSize) noexcept;
#endif // TRIVIAL_PLATFORM_LINUX

[[nodiscard]] void* reserveAligned(std::size_t bytes, std::size_t alignment, int& outOsErrorCode) noexcept;

void releaseReservation(void* base, std::size_t bytes) noexcept;

[[nodiscard]] bool commitPages(void* addr, std::size_t bytes, std::size_t pageSize, int& outOsErrorCode) noexcept;

#if TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
bool commitLargePages(void* addr, std::size_t bytes, std::size_t largePageSize, int& outOsErrorCode) noexcept;
#endif // TRIVIAL_MEMORY_ENABLE_LARGE_PAGES

#if TRIVIAL_MEMORY_ENABLE_DECOMMIT
void decommitPages(void* addr, std::size_t bytes, std::size_t pageSize) noexcept;
#endif // TRIVIAL_MEMORY_ENABLE_DECOMMIT

} // namespace trivial::memory

#endif // TRIVIAL_SRC_CORE_MEMORY_VIRTUAL_MEMORY_H

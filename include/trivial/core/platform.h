#ifndef TRIVIAL_CORE_PLATFORM_H
#define TRIVIAL_CORE_PLATFORM_H

#include <cstddef>

#if defined(_WIN32)
#define TRIVIAL_PLATFORM_WINDOWS 1
#define TRIVIAL_PLATFORM_LINUX 0
#define TRIVIAL_PLATFORM_MACOS 0

#elif defined(__APPLE__)
#include <TargetConditionals.h>

#if TARGET_OS_OSX
#define TRIVIAL_PLATFORM_WINDOWS 0
#define TRIVIAL_PLATFORM_LINUX 0
#define TRIVIAL_PLATFORM_MACOS 1

#else
#error "Unsupported Apple platform"

#endif // Check apple device type

#elif defined(__linux__)
#define TRIVIAL_PLATFORM_WINDOWS 0
#define TRIVIAL_PLATFORM_LINUX 1
#define TRIVIAL_PLATFORM_MACOS 0

#else
#error "Unsupported platform"

#endif // Platform check

#define TRIVIAL_PLATFORM_POSIX (TRIVIAL_PLATFORM_LINUX || TRIVIAL_PLATFORM_MACOS)

#if defined(_M_X64) || defined(__x86_64__)
#define TRIVIAL_ARCH_X86_64 1
#define TRIVIAL_ARCH_ARM64 0

#elif defined(_M_ARM64) || defined(__aarch64__)
#define TRIVIAL_ARCH_X86_64 0
#define TRIVIAL_ARCH_ARM64 1

#else
#error "Unsupported CPU architecture"

#endif // Cpu architecture check

#if TRIVIAL_PLATFORM_MACOS && TRIVIAL_ARCH_ARM64
#define TRIVIAL_PLATFORM_PAGE_SIZE_KNOWN 1
#define TRIVIAL_PLATFORM_PAGE_SIZE 16384

#elif TRIVIAL_PLATFORM_MACOS && TRIVIAL_ARCH_X86_64
#define TRIVIAL_PLATFORM_PAGE_SIZE_KNOWN 1
#define TRIVIAL_PLATFORM_PAGE_SIZE 4096

#elif TRIVIAL_PLATFORM_WINDOWS
#define TRIVIAL_PLATFORM_PAGE_SIZE_KNOWN 1
#define TRIVIAL_PLATFORM_PAGE_SIZE 4096

#elif TRIVIAL_PLATFORM_LINUX && TRIVIAL_ARCH_X86_64
#define TRIVIAL_PLATFORM_PAGE_SIZE_KNOWN 1
#define TRIVIAL_PLATFORM_PAGE_SIZE 4096

#else
#define TRIVIAL_PLATFORM_PAGE_SIZE_KNOWN 0 // Rutime query
#define TRIVIAL_PLATFORM_PAGE_SIZE 4096

#endif // Page size

#if TRIVIAL_PLATFORM_WINDOWS
#define TRIVIAL_PLATFORM_ALLOCATION_GRANULARITY 65536

#else
#define TRIVIAL_PLATFORM_ALLOCATION_GRANULARITY TRIVIAL_PLATFORM_PAGE_SIZE

#endif // Allocation granularity

#if TRIVIAL_PLATFORM_MACOS && TRIVIAL_ARCH_ARM64
#define TRIVIAL_PLATFORM_CACHE_LINE_SIZE 128

#else
#define TRIVIAL_PLATFORM_CACHE_LINE_SIZE 64

#endif // Cache line size

// Adjacent line prefetcher safety
#define TRIVIAL_PLATFORM_FALSE_SHARING_ALIGNMENT 128

#if TRIVIAL_PLATFORM_WINDOWS
#if defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0A00)
#define TRIVIAL_PLATFORM_SDK_HAS_VIRTUAL_ALLOC2 1

#else
#define TRIVIAL_PLATFORM_SDK_HAS_VIRTUAL_ALLOC2 0

#endif // Windows version check

#else
#define TRIVIAL_PLATFORM_SDK_HAS_VIRTUAL_ALLOC2 0

#endif // TRIVIAL_PLATFORM_WINDOWS

#if TRIVIAL_PLATFORM_LINUX || TRIVIAL_PLATFORM_MACOS
#define TRIVIAL_PLATFORM_SDK_HAS_MADV_FREE 1

#else
#define TRIVIAL_PLATFORM_SDK_HAS_MADV_FREE 0

#endif // MADV_FREE availability

namespace trivial::core {

inline constexpr std::size_t g_kPageSize = TRIVIAL_PLATFORM_PAGE_SIZE;
inline constexpr std::size_t g_kAllocationGranularity = TRIVIAL_PLATFORM_ALLOCATION_GRANULARITY;
inline constexpr std::size_t g_kCacheLineSize = TRIVIAL_PLATFORM_CACHE_LINE_SIZE;
inline constexpr std::size_t g_kFalseSharingAlignment = TRIVIAL_PLATFORM_FALSE_SHARING_ALIGNMENT;

static_assert((g_kPageSize & (g_kPageSize - 1)) == 0, "Page size must be a power of two");
static_assert((g_kCacheLineSize & (g_kCacheLineSize - 1)) == 0, "Cache line size must be a power of two");
static_assert(g_kFalseSharingAlignment >= g_kCacheLineSize, "False sharing alignment must cover a cache line");

} // namespace trivial::core

static_assert(sizeof(void*) == 8, "Trivial targets 64-bit platforms only");

#endif // TRIVIAL_CORE_PLATFORM_H

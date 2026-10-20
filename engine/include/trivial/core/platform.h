#ifndef TRIVIAL_CORE_PLATFORM_H
#define TRIVIAL_CORE_PLATFORM_H

#ifdef _WIN32
#define TRIVIAL_PLATFORM_WINDOWS 1
#define TRIVIAL_PLATFORM_LINUX 0
#define TRIVIAL_PLATFORM_MACOS 0

#define TRIVIAL_PLATFORM_WINDOWS_MIN_VERSION 0x0A00           // Windows 10
#define TRIVIAL_PLATFORM_WINDOWS_MIN_NTDDI_VERSION 0x0A000005 // Windows 10 1803

#if !defined(_WIN32_WINNT) || _WIN32_WINNT < TRIVIAL_PLATFORM_WINDOWS_MIN_VERSION
#error "Target Windows SDK version is below TRIVIAL_PLATFORM_WINDOWS_MIN_VERSION"
#endif

#if !defined(NTDDI_VERSION) || NTDDI_VERSION < TRIVIAL_PLATFORM_WINDOWS_MIN_NTDDI_VERSION
#error "Target Windows SDK version is below TRIVIAL_PLATFORM_WINDOWS_MIN_NTDDI_VERSION"
#endif

// Record of all minimums (NTDDI_VERSION) for reference for future changes:
//     VirtualAlloc2 - 0x0A000005 (Windows 10, 1803+)
//     CREATE_WAITABLE_TIMER_HIGH_RESOLUTION - 0x0A000005 (Windows 10, 1803+)
//     SetThreadDescription - 0x0A000002 (Windows 10, 1607+)
//     WaitOnAddress - 0x06020000 (Windows 8)

#elifdef __APPLE__
#include <Availability.h>       // IWYU pragma: keep
#include <TargetConditionals.h> // IWYU pragma: keep

#if TARGET_OS_OSX
#define TRIVIAL_PLATFORM_WINDOWS 0
#define TRIVIAL_PLATFORM_LINUX 0
#define TRIVIAL_PLATFORM_MACOS 1

#define TRIVIAL_PLATFORM_MACOS_MIN_VERSION 140400 // macOS 14.4

#if !defined(__MAC_OS_X_VERSION_MIN_REQUIRED) || __MAC_OS_X_VERSION_MIN_REQUIRED < TRIVIAL_PLATFORM_MACOS_MIN_VERSION
#error "Target macOS deployment version is below TRIVIAL_PLATFORM_MACOS_MIN_VERSION"
#endif

// Record of all minimums for reference for future changes:
//     os_sync_wait_on_address - 140400 (macOS 14.4)

#else
#error "Unsupported Apple platform"

#endif // Check apple device type

#elifdef __linux__
#define TRIVIAL_PLATFORM_WINDOWS 0
#define TRIVIAL_PLATFORM_LINUX 1
#define TRIVIAL_PLATFORM_MACOS 0

// Record of all minimums for reference for future changes:
//     MADV_FREE - Linux 4.5

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
#define TRIVIAL_PLATFORM_PAGE_SIZE 16384U

#elif TRIVIAL_PLATFORM_MACOS && TRIVIAL_ARCH_X86_64
#define TRIVIAL_PLATFORM_PAGE_SIZE_KNOWN 1
#define TRIVIAL_PLATFORM_PAGE_SIZE 4096U

#elif TRIVIAL_PLATFORM_WINDOWS
#define TRIVIAL_PLATFORM_PAGE_SIZE_KNOWN 1
#define TRIVIAL_PLATFORM_PAGE_SIZE 4096U

#elif TRIVIAL_PLATFORM_LINUX && TRIVIAL_ARCH_X86_64
#define TRIVIAL_PLATFORM_PAGE_SIZE_KNOWN 1
#define TRIVIAL_PLATFORM_PAGE_SIZE 4096U

#else
#define TRIVIAL_PLATFORM_PAGE_SIZE_KNOWN 0 // Runtime query
#define TRIVIAL_PLATFORM_PAGE_SIZE 4096U

#endif // Page size

#if TRIVIAL_PLATFORM_WINDOWS
#define TRIVIAL_PLATFORM_ALLOCATION_GRANULARITY 65536U

#else
#define TRIVIAL_PLATFORM_ALLOCATION_GRANULARITY TRIVIAL_PLATFORM_PAGE_SIZE

#endif // Allocation granularity

#if TRIVIAL_PLATFORM_MACOS && TRIVIAL_ARCH_ARM64
#define TRIVIAL_PLATFORM_CACHE_LINE_SIZE 128U

#else
#define TRIVIAL_PLATFORM_CACHE_LINE_SIZE 64U

#endif // Cache line size

#define TRIVIAL_PLATFORM_FALSE_SHARING_ALIGNMENT 128U

#if TRIVIAL_PLATFORM_LINUX || TRIVIAL_PLATFORM_MACOS
#define TRIVIAL_PLATFORM_SDK_HAS_MADV_FREE 1

#else
#define TRIVIAL_PLATFORM_SDK_HAS_MADV_FREE 0

#endif // MADV_FREE availability

#if TRIVIAL_PLATFORM_LINUX || TRIVIAL_PLATFORM_WINDOWS
#define TRIVIAL_PLATFORM_HAS_CPU_AFFINITY 1

#else
#define TRIVIAL_PLATFORM_HAS_CPU_AFFINITY 0

#endif // CPU affinity availability

static_assert((TRIVIAL_PLATFORM_PAGE_SIZE & (TRIVIAL_PLATFORM_PAGE_SIZE - 1)) == 0, "Page size must be a power of two");
static_assert((TRIVIAL_PLATFORM_CACHE_LINE_SIZE & (TRIVIAL_PLATFORM_CACHE_LINE_SIZE - 1)) == 0,
              "Cache line size must be a power of two");
static_assert(TRIVIAL_PLATFORM_FALSE_SHARING_ALIGNMENT >= TRIVIAL_PLATFORM_CACHE_LINE_SIZE,
              "False sharing alignment must cover a cache line");

static_assert(sizeof(void*) == 8, "Trivial targets 64-bit platforms only"); // NOLINT(readability-magic-numbers)

#endif // TRIVIAL_CORE_PLATFORM_H

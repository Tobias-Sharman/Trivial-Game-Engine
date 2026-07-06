#ifndef TRIVIAL_CORE_PLATFORM_H
#define TRIVIAL_CORE_PLATFORM_H

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

#if defined(_M_X64) || defined(__x86_64__)
#define TRIVIAL_ARCH_X86_64 1
#define TRIVIAL_ARCH_ARM64 0

#elif defined(_M_ARM64) || defined(__aarch64__)
#define TRIVIAL_ARCH_X86_64 0
#define TRIVIAL_ARCH_ARM64 1

#else
#error "Unsupported CPU architecture"

#endif // Cpu architecture check

#endif // TRIVIAL_CORE_PLATFORM_H

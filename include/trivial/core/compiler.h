#ifndef TRIVIAL_CORE_COMPILER_H
#define TRIVIAL_CORE_COMPILER_H

#if defined(__clang__)
#define TRIVIAL_COMPILER_CLANG 1
#define TRIVIAL_COMPILER_GCC 0
#define TRIVIAL_COMPILER_MSVC 0

#elif defined(_MSC_VER)
#define TRIVIAL_COMPILER_CLANG 0
#define TRIVIAL_COMPILER_GCC 0
#define TRIVIAL_COMPILER_MSVC 1

#elif defined(__GNUC__)
#define TRIVIAL_COMPILER_CLANG 0
#define TRIVIAL_COMPILER_GCC 1
#define TRIVIAL_COMPILER_MSVC 0

#else
#error "Unsupported compiler"

#endif // Compiler check

#if TRIVIAL_COMPILER_CLANG && defined(_MSC_VER)
#define TRIVIAL_COMPILER_CLANG_CL 1

#else
#define TRIVIAL_COMPILER_CLANG_CL 0

#endif // For Clang on windows

#if defined(_MSC_VER)
#define TRIVIAL_FORCE_INLINE __forceinline
#define TRIVIAL_NO_INLINE __declspec(noinline)

#elif TRIVIAL_COMPILER_CLANG || TRIVIAL_COMPILER_GCC
#define TRIVIAL_FORCE_INLINE inline __attribute__((always_inline))
#define TRIVIAL_NO_INLINE __attribute__((noinline))

#endif // Force inline macro

#if TRIVIAL_COMPILER_MSVC || TRIVIAL_COMPILER_CLANG_CL
#define TRIVIAL_DEBUG_BREAK() __debugbreak()

#elif (TRIVIAL_COMPILER_CLANG || TRIVIAL_COMPILER_GCC) && (defined(__i386__) || defined(__x86_64__))
#define TRIVIAL_DEBUG_BREAK() __asm__ volatile("int3")

#elif (TRIVIAL_COMPILER_CLANG || TRIVIAL_COMPILER_GCC) && defined(__aarch64__)
#define TRIVIAL_DEBUG_BREAK() __asm__ volatile("brk #0")

#else
#define TRIVIAL_DEBUG_BREAK() __builtin_trap()

#endif // Debug break

#endif // TRIVIAL_CORE_COMPILER_H

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

#endif // TRIVIAL_CORE_COMPILER_H

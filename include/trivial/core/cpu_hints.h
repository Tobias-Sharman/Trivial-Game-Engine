#ifndef TRIVIAL_CORE_CPU_HINTS_H
#define TRIVIAL_CORE_CPU_HINTS_H

#include <trivial/core/compiler.h>
#include <trivial/core/platform.h>

#if TRIVIAL_ARCH_X86_64
#include <immintrin.h>
#define TRIVIAL_CPU_PAUSE() _mm_pause()

#elif TRIVIAL_ARCH_ARM64
#if TRIVIAL_COMPILER_MSVC
#include <intrin.h>
#define TRIVIAL_CPU_PAUSE() __yield()

#elif TRIVIAL_COMPILER_CLANG
#include <arm_acle.h>
#define TRIVIAL_CPU_PAUSE() __yield()

#elif TRIVIAL_COMPILER_GCC
#define TRIVIAL_CPU_PAUSE() __asm__ volatile("yield" ::: "memory")

#else
#error "Unsupported compiler"

#endif // Compiler check

#else
#error "Unsupported cpu architecture"

#endif // Architecture check

#endif // TRIVIAL_CORE_CPU_HINTS_H

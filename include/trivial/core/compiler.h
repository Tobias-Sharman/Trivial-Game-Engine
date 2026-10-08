#ifndef TRIVIAL_CORE_COMPILER_H
#define TRIVIAL_CORE_COMPILER_H

#ifdef __clang__
#define TRIVIAL_COMPILER_CLANG 1
#define TRIVIAL_COMPILER_GCC 0
#define TRIVIAL_COMPILER_MSVC 0

#elifdef _MSC_VER
#define TRIVIAL_COMPILER_CLANG 0
#define TRIVIAL_COMPILER_GCC 0
#define TRIVIAL_COMPILER_MSVC 1

#elifdef __GNUC__
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

#ifdef _MSC_VER
#define TRIVIAL_FORCE_INLINE inline __forceinline
#define TRIVIAL_NO_INLINE __declspec(noinline)
#define TRIVIAL_COLD
#define TRIVIAL_HOT

#elif TRIVIAL_COMPILER_CLANG || TRIVIAL_COMPILER_GCC
#define TRIVIAL_FORCE_INLINE inline __attribute__((always_inline))
#define TRIVIAL_NO_INLINE __attribute__((noinline))
#define TRIVIAL_COLD __attribute__((cold))
#define TRIVIAL_HOT __attribute__((hot))

#endif // Force inline macro

#if __has_cpp_attribute(msvc::flatten)
#define TRIVIAL_FLATTEN [[msvc::flatten]]

#elif TRIVIAL_COMPILER_CLANG || TRIVIAL_COMPILER_GCC
#define TRIVIAL_FLATTEN __attribute__((flatten))

#else
#define TRIVIAL_FLATTEN

#endif // Flatten

#define TRIVIAL_RESTRICT __restrict

#if __has_cpp_attribute(clang::lifetimebound)
#define TRIVIAL_LIFETIMEBOUND [[clang::lifetimebound]]

#elif __has_cpp_attribute(msvc::lifetimebound)
#define TRIVIAL_LIFETIMEBOUND [[msvc::lifetimebound]]

#else
#define TRIVIAL_LIFETIMEBOUND

#endif // Lifetime bound

#if __has_cpp_attribute(clang::reinitializes)
#define TRIVIAL_REINITIALISES [[clang::reinitializes]]

#else
#define TRIVIAL_REINITIALISES

#endif // Reinitialises

#ifdef _MSC_VER
#define TRIVIAL_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]

#else
#define TRIVIAL_NO_UNIQUE_ADDRESS [[no_unique_address]]

#endif // No unique address

#ifdef _MSC_VER
#define TRIVIAL_NOVTABLE __declspec(novtable)
#define TRIVIAL_EMPTY_BASES __declspec(empty_bases)

#else
#define TRIVIAL_NOVTABLE
#define TRIVIAL_EMPTY_BASES

#endif // MSVC class layout

#if TRIVIAL_COMPILER_MSVC || TRIVIAL_COMPILER_CLANG_CL
#define TRIVIAL_DEBUG_BREAK() __debugbreak()

#elif (TRIVIAL_COMPILER_CLANG || TRIVIAL_COMPILER_GCC) && (defined(__i386__) || defined(__x86_64__))
#define TRIVIAL_DEBUG_BREAK() __asm__ volatile("int3") // NOLINT(portability-no-assembler)

#elif (TRIVIAL_COMPILER_CLANG || TRIVIAL_COMPILER_GCC) && defined(__aarch64__)
#define TRIVIAL_DEBUG_BREAK() __asm__ volatile("brk #0") // NOLINT(portability-no-assembler)

#else
#define TRIVIAL_DEBUG_BREAK() __builtin_trap()

#endif // Debug break

#define TRIVIAL_DIAGNOSTIC_PRAGMA_STRING(text) #text

#if TRIVIAL_COMPILER_CLANG
#define TRIVIAL_DIAGNOSTIC_PUSH _Pragma("clang diagnostic push")
#define TRIVIAL_DIAGNOSTIC_POP _Pragma("clang diagnostic pop")
#define TRIVIAL_DIAGNOSTIC_IGNORE_CLANG(warning)                                                                       \
	_Pragma(TRIVIAL_DIAGNOSTIC_PRAGMA_STRING(clang diagnostic ignored warning))
#define TRIVIAL_DIAGNOSTIC_IGNORE_GCC(warning)
#define TRIVIAL_DIAGNOSTIC_IGNORE_MSVC(number)

#elif TRIVIAL_COMPILER_GCC
#define TRIVIAL_DIAGNOSTIC_PUSH _Pragma("GCC diagnostic push")
#define TRIVIAL_DIAGNOSTIC_POP _Pragma("GCC diagnostic pop")
#define TRIVIAL_DIAGNOSTIC_IGNORE_CLANG(warning)
#define TRIVIAL_DIAGNOSTIC_IGNORE_GCC(warning) _Pragma(TRIVIAL_DIAGNOSTIC_PRAGMA_STRING(GCC diagnostic ignored warning))
#define TRIVIAL_DIAGNOSTIC_IGNORE_MSVC(number)

#elif TRIVIAL_COMPILER_MSVC
#define TRIVIAL_DIAGNOSTIC_PUSH __pragma(warning(push))
#define TRIVIAL_DIAGNOSTIC_POP __pragma(warning(pop))
#define TRIVIAL_DIAGNOSTIC_IGNORE_CLANG(warning)
#define TRIVIAL_DIAGNOSTIC_IGNORE_GCC(warning)
#define TRIVIAL_DIAGNOSTIC_IGNORE_MSVC(number) __pragma(warning(disable : number))

#endif // Diagnostic suppression

#ifdef __SANITIZE_THREAD__
#define TRIVIAL_THREAD_SANITISER_ENABLED 1

#elifdef __has_feature
#if __has_feature(thread_sanitizer)
#define TRIVIAL_THREAD_SANITISER_ENABLED 1
#endif // __has_feature(thread_sanitizer)

#endif // defined(__has_feature)

#ifndef TRIVIAL_THREAD_SANITISER_ENABLED
#define TRIVIAL_THREAD_SANITISER_ENABLED 0

#endif // TRIVIAL_THREAD_SANITISER_ENABLED

#endif // TRIVIAL_CORE_COMPILER_H

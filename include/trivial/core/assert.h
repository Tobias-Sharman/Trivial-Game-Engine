#ifndef TRIVIAL_CORE_ASSERT_H
#define TRIVIAL_CORE_ASSERT_H

#include <trivial/core/config.h>

#if !TRIVIAL_ENABLE_ASSERTS
#include <utility> // IWYU pragma: keep

#include <trivial/core/compiler.h>
#endif // !TRIVIAL_ENABLE_ASSERTS

#if TRIVIAL_ENABLE_ASSERTS || TRIVIAL_ENABLE_SLOW_ASSERTS

#include <source_location>

namespace trivial::core {

[[noreturn]] void reportAssertionFailure(const char* expression,
                                         std::source_location location = std::source_location::current()) noexcept;

} // namespace trivial::core

#define TRIVIAL_ASSERT_IMPLEMENTATION(expr)                                                                            \
	do {                                                                                                               \
		if (!(expr)) [[unlikely]] {                                                                                    \
			::trivial::core::reportAssertionFailure(#expr);                                                            \
		}                                                                                                              \
	} while (false)

#endif // TRIVIAL_ENABLE_ASSERTS || TRIVIAL_ENABLE_SLOW_ASSERTS

#if TRIVIAL_ENABLE_ASSERTS
#define TRIVIAL_ASSERT(expr) TRIVIAL_ASSERT_IMPLEMENTATION(expr)
#define TRIVIAL_VERIFY(expr) TRIVIAL_ASSERT_IMPLEMENTATION(expr)
#define TRIVIAL_ASSUME(expr) TRIVIAL_ASSERT_IMPLEMENTATION(expr)
#define TRIVIAL_UNREACHABLE() ::trivial::core::reportAssertionFailure("unreachable")

#else
#define TRIVIAL_ASSERT(expr) ((void)0)
#define TRIVIAL_VERIFY(expr) ((void)(expr))
#define TRIVIAL_UNREACHABLE() std::unreachable()

#if TRIVIAL_COMPILER_MSVC
#define TRIVIAL_ASSUME(expr) __assume(expr)

#else
#define TRIVIAL_ASSUME(expr) [[assume(expr)]]

#endif // Assume

#endif // TRIVIAL_ENABLE_ASSERTS

#if TRIVIAL_ENABLE_SLOW_ASSERTS
#define TRIVIAL_SLOW_ASSERT(expr) TRIVIAL_ASSERT_IMPLEMENTATION(expr)

#else
#define TRIVIAL_SLOW_ASSERT(expr) ((void)0)

#endif // TRIVIAL_ENABLE_SLOW_ASSERTS

#endif // TRIVIAL_CORE_ASSERT_H

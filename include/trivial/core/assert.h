#ifndef TRIVIAL_CORE_ASSERT_H
#define TRIVIAL_CORE_ASSERT_H

#include <trivial/core/compiler.h>
#include <trivial/core/config.h>

#if TRIVIAL_ENABLE_ASSERTS || TRIVIAL_ENABLE_SLOW_ASSERTS

#include <source_location>

namespace trivial::core {

[[noreturn]] void reportAssertionFailure(const char* expression,
                                         std::source_location location = std::source_location::current()) noexcept;

} // namespace trivial::core

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TRIVIAL_ASSERT_IMPLEMENTATION(expr)                                                                            \
	do {                                                                                                               \
		if (!(expr)) [[unlikely]] {                                                                                    \
			::trivial::core::reportAssertionFailure(#expr);                                                            \
		}                                                                                                              \
	} while (false)

#endif // TRIVIAL_ENABLE_ASSERTS || TRIVIAL_ENABLE_SLOW_ASSERTS

#if TRIVIAL_ENABLE_ASSERTS
#define TRIVIAL_ASSERT(expr) TRIVIAL_ASSERT_IMPLEMENTATION(expr) // NOLINT(cppcoreguidelines-macro-usage)

#else
#define TRIVIAL_ASSERT(expr) ((void)0) // NOLINT(cppcoreguidelines-macro-usage)

#endif // TRIVIAL_ENABLE_ASSERTS

#if TRIVIAL_ENABLE_SLOW_ASSERTS
#define TRIVIAL_SLOW_ASSERT(expr) TRIVIAL_ASSERT_IMPLEMENTATION(expr) // NOLINT(cppcoreguidelines-macro-usage)

#else
#define TRIVIAL_SLOW_ASSERT(expr) ((void)0) // NOLINT(cppcoreguidelines-macro-usage)

#endif // TRIVIAL_ENABLE_SLOW_ASSERTS

#endif // TRIVIAL_CORE_ASSERT_H

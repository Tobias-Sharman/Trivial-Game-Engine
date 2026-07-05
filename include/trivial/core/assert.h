#ifndef TRIVIAL_SRC_CORE_ASSERT_H
#define TRIVIAL_SRC_CORE_ASSERT_H

#include <trivial/core/config.h>

#if TRIVIAL_ENABLE_ASSERTS || TRIVIAL_ENABLE_SLOW_ASSERTS

#include <cstdlib>
#include <iostream>
#include <source_location>

namespace trivial::core::internal {

inline void reportAssertionFailure(const char* expression,
                                   std::source_location location = std::source_location::current()) {
	std::cerr << "Trivial assertion failed: " << expression << '\n'
	          << "File: " << location.file_name() << '\n'
	          << "Line: " << location.line() << '\n'
	          << "Function: " << location.function_name() << '\n';

	std::abort();
}

} // namespace trivial::core::internal

#define TRIVIAL_INTERNAL_ASSERT_IMPLEMENTATION(expr)                                                                   \
	do {                                                                                                               \
		if (!(expr)) {                                                                                                 \
			::trivial::core::internal::reportAssertionFailure(#expr);                                                  \
		}                                                                                                              \
	} while (false)

#endif // TRIVIAL_ENABLE_ASSERTS || TRIVIAL_ENABLE_SLOW_ASSERTS

#if TRIVIAL_ENABLE_ASSERTS

#define TRIVIAL_ASSERT(expr) TRIVIAL_INTERNAL_ASSERT_IMPLEMENTATION(expr) // NOLINT(cppcoreguidelines-macro-usage)

#else

#define TRIVIAL_ASSERT(expr) ((void)0) // NOLINT(cppcoreguidelines-macro-usage)

#endif // TRIVIAL_ENABLE_ASSERTS

#if TRIVIAL_ENABLE_SLOW_ASSERTS

#define TRIVIAL_SLOW_ASSERT(expr) TRIVIAL_INTERNAL_ASSERT_IMPLEMENTATION(expr) // NOLINT(cppcoreguidelines-macro-usage)

#else

#define TRIVIAL_SLOW_ASSERT(expr) ((void)0) // NOLINT(cppcoreguidelines-macro-usage)

#endif // TRIVIAL_ENABLE__SLOW_ASSERTS

#endif // TRIVIAL_SRC_CORE_ASSERT_H

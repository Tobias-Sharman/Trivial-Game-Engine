#include <trivial/core/assert.h>

#if TRIVIAL_ENABLE_ASSERTS || TRIVIAL_ENABLE_SLOW_ASSERTS

#include <cstdio>
#include <cstdlib>

namespace trivial::core {

[[noreturn]] void reportAssertionFailure(const char* expression, std::source_location location) noexcept {
	// TODO: route through the logging system once it exists
	(void)std::fprintf(stderr,
	                   "Trivial assertion failed: %s\nFile: %s\nLine: %u\nFunction: %s\n",
	                   expression,
	                   location.file_name(),
	                   location.line(),
	                   location.function_name());
	(void)std::fflush(stderr);

	TRIVIAL_DEBUG_BREAK();

	std::abort();
}

} // namespace trivial::core

#endif // TRIVIAL_ENABLE_ASSERTS || TRIVIAL_ENABLE_SLOW_ASSERTS

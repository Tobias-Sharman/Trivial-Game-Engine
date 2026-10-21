#include "core/time/time_capabilities.h"

#include <cstdint>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX

#include <windows.h> // IWYU pragma: keep

#include <trivial/core/assert.h>
#include <trivial/core/time/time.h>

namespace {

[[nodiscard]] std::uint64_t probePerformanceFrequency() noexcept {
	LARGE_INTEGER frequency{};
	TRIVIAL_VERIFY(QueryPerformanceFrequency(&frequency) != 0);

	return static_cast<std::uint64_t>(frequency.QuadPart);
}

} // namespace

namespace trivial::time {

void initCapabilities() noexcept {
	detail::g_detectedCapabilities = detail::DetectedCapabilities{.ticksPerSecond = probePerformanceFrequency()};
}

} // namespace trivial::time

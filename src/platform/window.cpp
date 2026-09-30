#include <trivial/platform/window.h>

#include <trivial/core/assert.h>
#include <trivial/platform/window_types.h>

namespace trivial::platform {

namespace {

WindowConfig readWindowConfig(const WindowConfig& config) noexcept {
	TRIVIAL_ASSERT(config.size.width > 0);
	TRIVIAL_ASSERT(config.size.height > 0);
	TRIVIAL_ASSERT(!config.title.empty());

	return config;
}

} // namespace

Window::Window(const WindowConfig& config) noexcept
    : m_config(readWindowConfig(config))
    , m_backend(m_config) {
}

} // namespace trivial::platform

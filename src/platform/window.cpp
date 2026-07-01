#include <trivial/internal/platform/window.h>

#include "core/assert.h"

namespace trivial::internal::platform {

namespace {

WindowConfig readWindowConfig(const EngineConfig* config) {
	TRIVIAL_ASSERT(config != nullptr);
	TRIVIAL_ASSERT(config->window.size.height > 0);
	TRIVIAL_ASSERT(config->window.size.width > 0);
	TRIVIAL_ASSERT(!config->window.title.empty());

	return config->window;
}

} // namespace

Window::Window(const EngineConfig* config)
    : m_config(readWindowConfig(config))
    , m_backend(&m_config) {
}

// Expected to get more complex so this is going here
Window::~Window() = default;

} // namespace trivial::internal::platform

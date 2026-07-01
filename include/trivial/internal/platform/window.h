#ifndef TRIVIAL_INTERNAL_PLATFORM_WINDOW_H
#define TRIVIAL_INTERNAL_PLATFORM_WINDOW_H

#include <span>

#include <vulkan/vulkan.h>

#include <trivial/engine_config.h>
#include <trivial/internal/platform/window_backend.h>

namespace trivial::internal::platform {

class Window {
public:
	Window() = delete;
	explicit Window(const EngineConfig* config);

	~Window();

	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	Window(Window&&) = delete;
	Window& operator=(Window&&) = delete;

	static void pollEvents() { WindowBackend::pollEvents(); }

	[[nodiscard]] bool shouldClose() const { return m_backend.shouldClose(); }

	[[nodiscard]] static std::span<const char* const> requiredVulkanInstanceExtensions() {
		return WindowBackend::requiredVulkanInstanceExtensions();
	}

	[[nodiscard]] VkSurfaceKHR createVulkanSurface(VkInstance instance) const {
		return m_backend.createVulkanSurface(instance);
	}

private:
	WindowConfig m_config;
	WindowBackend m_backend;
};

} // namespace trivial::internal::platform

#endif // TRIVIAL_INTERNAL_PLATFORM_WINDOW_H

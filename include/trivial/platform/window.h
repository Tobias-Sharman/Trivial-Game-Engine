#ifndef TRIVIAL_PLATFORM_WINDOW_H
#define TRIVIAL_PLATFORM_WINDOW_H

#include <span>

#include <vulkan/vulkan.h>

#include <trivial/engine_config.h>
#include <trivial/platform/window_backend.h>

namespace trivial::platform {

class Window {
public:
	explicit Window(const EngineConfig* config) noexcept;

	~Window();

	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	Window(Window&&) = delete;
	Window& operator=(Window&&) = delete;

	static void pollEvents() noexcept { WindowBackend::pollEvents(); }

	[[nodiscard]] bool shouldClose() const noexcept { return m_backend.shouldClose(); }

	[[nodiscard]] WindowSize framebufferSize() const noexcept { return m_backend.framebufferSize(); }

	[[nodiscard]] static std::span<const char* const> requiredVulkanInstanceExtensions() noexcept {
		return WindowBackend::requiredVulkanInstanceExtensions();
	}

	[[nodiscard]] VkSurfaceKHR createVulkanSurface(VkInstance instance) const noexcept {
		return m_backend.createVulkanSurface(instance);
	}

private:
	WindowConfig m_config;
	WindowBackend m_backend;
};

} // namespace trivial::platform

#endif // TRIVIAL_PLATFORM_WINDOW_H

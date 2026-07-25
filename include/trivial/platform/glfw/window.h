#ifndef TRIVIAL_PLATFORM_GLFW_WINDOW_H
#define TRIVIAL_PLATFORM_GLFW_WINDOW_H

#include <span>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include <trivial/engine_config.h>

namespace trivial::platform::glfw {

class Window {
public:
	explicit Window(const WindowConfig* config) noexcept;

	// TODO: Move GLFW init/terminate into a GLFW runtime object for multiple window support
	~Window();

	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	Window(Window&&) = delete;
	Window& operator=(Window&&) = delete;

	// TODO: Move event polling to a platform window system/runtime for multiple window support
	static void pollEvents() noexcept { glfwPollEvents(); }

	[[nodiscard]] bool shouldClose() const noexcept;

	[[nodiscard]] WindowSize framebufferSize() const noexcept;

	[[nodiscard]] static std::span<const char* const> requiredVulkanInstanceExtensions() noexcept;

	[[nodiscard]] VkSurfaceKHR createVulkanSurface(VkInstance instance) const noexcept;

private:
	GLFWwindow* m_handle = nullptr;
};

} // namespace trivial::platform::glfw

#endif // TRIVIAL_PLATFORM_GLFW_WINDOW_H

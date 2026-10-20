#ifndef TRIVIAL_PLATFORM_GLFW_WINDOW_H
#define TRIVIAL_PLATFORM_GLFW_WINDOW_H

#include <span>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#undef GLFW_INCLUDE_VULKAN

#include <vulkan/vulkan_core.h>

#include <trivial/platform/window_types.h>

namespace trivial::platform::glfw {

class Window {
public:
	explicit Window(const WindowConfig& config) noexcept;

	~Window();

	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	Window(Window&&) = delete;
	Window& operator=(Window&&) = delete;

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

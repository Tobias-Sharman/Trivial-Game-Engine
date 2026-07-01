#ifndef TRIVIAL_INTERNAL_PLATFORM_GLFW_WINDOW_H
#define TRIVIAL_INTERNAL_PLATFORM_GLFW_WINDOW_H

#include <span>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include <trivial/engine_config.h>

namespace trivial::internal::platform::glfw {

class Window {
public:
	Window() = delete;
	explicit Window(const WindowConfig* config);

	~Window();

	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	Window(Window&&) = delete;
	Window& operator=(Window&&) = delete;

	static void pollEvents();

	[[nodiscard]] bool shouldClose() const;

	[[nodiscard]] static std::span<const char* const> requiredVulkanInstanceExtensions();

	[[nodiscard]] VkSurfaceKHR createVulkanSurface(VkInstance instance) const;

private:
	GLFWwindow* m_handle = nullptr;
};

} // namespace trivial::internal::platform::glfw

#endif // TRIVIAL_INTERNAL_PLATFORM_GLFW_WINDOW_H

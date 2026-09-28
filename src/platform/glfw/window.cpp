#include <trivial/platform/glfw/window.h>

#include <cstdint>

#include <trivial/core/assert.h>
#include <trivial/core/config.h>
#include <trivial/core/log.h>

#if TRIVIAL_ENABLE_ASSERTS
#include "rhi/vulkan/result.h"
#endif // TRIVIAL_ENABLE_ASSERTS

namespace trivial::platform::glfw {

namespace {

void initializeGlfw() noexcept {
#if TRIVIAL_ENABLE_ASSERTS
	const int kResult = glfwInit();

	TRIVIAL_ASSERT(kResult == GLFW_TRUE);
#else
	glfwInit();
#endif // TRIVIAL_ENABLE_ASSERTS
}

} // namespace

Window::Window(const WindowConfig& config) noexcept {
	TRIVIAL_ASSERT(config.size.height > 0);
	TRIVIAL_ASSERT(config.size.width > 0);
	TRIVIAL_ASSERT(!config.title.empty());

	initializeGlfw();

	// NOTE: Good for vulkan need to look to adjust when adding more apis
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	m_handle = glfwCreateWindow(static_cast<int>(config.size.width),
	                            static_cast<int>(config.size.height),
	                            config.title.c_str(),
	                            nullptr,
	                            nullptr);

	TRIVIAL_ASSERT(m_handle != nullptr);
}

Window::~Window() {
	if (m_handle != nullptr) {
		glfwDestroyWindow(m_handle);
		m_handle = nullptr;
	}

	glfwTerminate();
}

bool Window::shouldClose() const noexcept {
	TRIVIAL_ASSERT(m_handle != nullptr);

	return glfwWindowShouldClose(m_handle) == GLFW_TRUE;
}

WindowSize Window::framebufferSize() const noexcept {
	int width = 0;
	int height = 0;
	glfwGetFramebufferSize(m_handle, &width, &height);

	TRIVIAL_ASSERT(width >= 0);
	TRIVIAL_ASSERT(height >= 0);

	return {.height = static_cast<std::uint32_t>(width), .width = static_cast<std::uint32_t>(height)};
}

std::span<const char* const> Window::requiredVulkanInstanceExtensions() noexcept {
#if TRIVIAL_ENABLE_ASSERTS
	const int kVulkanSupported = glfwVulkanSupported();

	TRIVIAL_ASSERT(kVulkanSupported == GLFW_TRUE);
#endif // TRIVIAL_ENABLE_ASSERTS

	std::uint32_t extensionCount = 0;
	const char** extensions = glfwGetRequiredInstanceExtensions(&extensionCount);

	TRIVIAL_ASSERT(extensions != nullptr);
	TRIVIAL_ASSERT(extensionCount > 0);

	return {extensions, static_cast<std::size_t>(extensionCount)};
}

VkSurfaceKHR Window::createVulkanSurface(VkInstance instance) const noexcept {
	TRIVIAL_ASSERT(instance != VK_NULL_HANDLE);
	TRIVIAL_ASSERT(m_handle != nullptr);

	VkSurfaceKHR surface = VK_NULL_HANDLE;

	const VkResult kResult = glfwCreateWindowSurface(instance, m_handle, nullptr, &surface);

	if (kResult != VK_SUCCESS) {
		TRIVIAL_LOG_ERROR("glfwCreateWindowSurface failed");
		TRIVIAL_LOG_ERROR(rhi::vulkan::resultName(kResult));
	}

	TRIVIAL_ASSERT(kResult == VK_SUCCESS);
	TRIVIAL_ASSERT(surface != VK_NULL_HANDLE);

	return surface;
}

} // namespace trivial::platform::glfw

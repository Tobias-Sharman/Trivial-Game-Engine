#include <trivial/internal/platform/glfw/window.h>

#include <cstdint>

#include <trivial/internal/rhi/vulkan/result.h>

#include "core/assert.h"
#include "core/log.h"

namespace trivial::internal::platform::glfw {

namespace {

void initializeGlfw() {
	const int kResult = glfwInit();

	TRIVIAL_ASSERT(kResult == GLFW_TRUE);
}

} // namespace

Window::Window(const WindowConfig* config) {
	TRIVIAL_ASSERT(config != nullptr);
	TRIVIAL_ASSERT(config->size.height > 0);
	TRIVIAL_ASSERT(config->size.width > 0);
	TRIVIAL_ASSERT(!config->title.empty());

	initializeGlfw();

	// TODO: Good for vulkan need to look to adjust when adding more apis
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	m_handle = glfwCreateWindow(config->size.width, config->size.height, config->title.c_str(), nullptr, nullptr);

	TRIVIAL_ASSERT(m_handle != nullptr);
}

// TODO: Move GLFW init/terminate into a GLFW runtime object for multiple window support
Window::~Window() {
	if (m_handle != nullptr) {
		glfwDestroyWindow(m_handle);
		m_handle = nullptr;
	}

	glfwTerminate();
}

// TODO: Move event polling to a platform window system/runtime for multiple window support
// NOTE: Keeping here in case of changes or added asserts
void Window::pollEvents() {
	glfwPollEvents();
}

bool Window::shouldClose() const {
	TRIVIAL_ASSERT(m_handle != nullptr);

	return glfwWindowShouldClose(m_handle) == GLFW_TRUE;
}

std::span<const char* const> Window::requiredVulkanInstanceExtensions() {
	const int kVulkanSupported = glfwVulkanSupported();

	TRIVIAL_ASSERT(kVulkanSupported == GLFW_TRUE);

	std::uint32_t extensionCount = 0;
	const char** extensions = glfwGetRequiredInstanceExtensions(&extensionCount);

	TRIVIAL_ASSERT(extensions != nullptr);
	TRIVIAL_ASSERT(extensionCount > 0);

	return {extensions, static_cast<std::size_t>(extensionCount)};
}

VkSurfaceKHR Window::createVulkanSurface(VkInstance instance) const {
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

} // namespace trivial::internal::platform::glfw

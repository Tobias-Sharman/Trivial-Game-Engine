#include <trivial/rhi/vulkan/backend.h>

#include <cstring>
#include <span>
#include <vector>

#include <trivial/core/assert.h>
#include <trivial/core/config.h>
#include <trivial/core/log.h>

#include "rhi/vulkan/device.h"
#include "rhi/vulkan/instance.h"
#include "rhi/vulkan/physical_device.h"
#include "rhi/vulkan/result.h"

#if TRIVIAL_ENABLE_VULKAN_VALIDATION

#include "rhi/vulkan/debug_messenger.h"

#endif // TRIVIAL_ENABLE_VULKAN_VALIDATION

namespace trivial::rhi::vulkan {

Backend::Backend(const EngineConfig* config, platform::Window* window)
    : m_instance(createInstance(config))
#if TRIVIAL_ENABLE_VULKAN_VALIDATION
    , m_debugMessenger(createDebugMessenger(m_instance))
#endif // TRIVIAL_ENABLE_VULKAN_VALIDATION
    , m_surface(window->createVulkanSurface(m_instance)) {
	const std::vector<VkPhysicalDevice> kPhysicalDevices = enumeratePhysicalDevices(m_instance);

	const PhysicalDeviceSelection kPhysicalDeviceSelection = selectPhysicalDevice(kPhysicalDevices, m_surface);

	m_physicalDevice = kPhysicalDeviceSelection.physicalDevice;
	m_graphicsFamily = kPhysicalDeviceSelection.queueFamilies.graphicsFamily;
	m_presentFamily = kPhysicalDeviceSelection.queueFamilies.presentFamily;

	m_device = createDevice(m_physicalDevice, &kPhysicalDeviceSelection.queueFamilies);

	m_graphicsQueue = getDeviceQueue(m_device, m_graphicsFamily);
	m_presentQueue = getDeviceQueue(m_device, m_presentFamily);
}

Backend::~Backend() {
	if (m_device != VK_NULL_HANDLE) {
		vkDestroyDevice(m_device, nullptr);
		m_device = VK_NULL_HANDLE;
	}

	m_graphicsQueue = VK_NULL_HANDLE;
	m_graphicsFamily = 0;

	m_presentQueue = VK_NULL_HANDLE;
	m_presentFamily = 0;

	m_physicalDevice = VK_NULL_HANDLE;

	if (m_surface != VK_NULL_HANDLE) {
		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
		m_surface = VK_NULL_HANDLE;
	}

#if TRIVIAL_ENABLE_VULKAN_VALIDATION
	destroyDebugMessenger(m_instance, m_debugMessenger);
	m_debugMessenger = VK_NULL_HANDLE;
#endif // TRIVIAL_ENABLE_VULKAN_VALIDATION

	if (m_instance != VK_NULL_HANDLE) {
		vkDestroyInstance(m_instance, nullptr);
		m_instance = VK_NULL_HANDLE;
	}
}

GraphicsApi Backend::graphicsApi() const {
	return GraphicsApi::Vulkan;
}

void Backend::waitIdle() {
	if (m_device == VK_NULL_HANDLE) {
		return;
	}

	const VkResult kResult = vkDeviceWaitIdle(m_device);

	if (kResult != VK_SUCCESS) {
		TRIVIAL_LOG_ERROR("vkDeviceWaitIdle failed");
		TRIVIAL_LOG_ERROR(resultName(kResult));
	}

	TRIVIAL_ASSERT(kResult == VK_SUCCESS);
}

} // namespace trivial::rhi::vulkan

#ifndef TRIVIAL_SRC_RHI_VULKAN_PHYSICAL_DEVICE_H
#define TRIVIAL_SRC_RHI_VULKAN_PHYSICAL_DEVICE_H

#include <cstdint>
#include <span>
#include <vector>

#include <vulkan/vulkan.h>

namespace trivial::rhi::vulkan {

struct QueueFamilySelection {
	std::uint32_t graphicsFamily = 0;
	std::uint32_t computeFamily = 0;
	std::uint32_t transferFamily = 0;
	std::uint32_t presentFamily = 0;

	bool hasGraphicsFamily = false;
	bool hasComputeFamily = false;
	bool hasTransferFamily = false;
	bool hasPresentFamily = false;
};

struct PhysicalDeviceSelection {
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	QueueFamilySelection queueFamilies = {};
};

struct DeviceFeatures {
	VkPhysicalDeviceFeatures2 features = {};
	VkPhysicalDeviceVulkan13Features vulkan13 = {};
};

PhysicalDeviceSelection selectPhysicalDevice(std::span<const VkPhysicalDevice> physicalDevices, VkSurfaceKHR surface);

bool hasDeviceExtension(std::span<const VkExtensionProperties> availableExtensions, const char* extensionName);
std::vector<VkExtensionProperties> enumerateDeviceExtensions(VkPhysicalDevice physicalDevice);
std::vector<VkPhysicalDevice> enumeratePhysicalDevices(VkInstance instance);
DeviceFeatures makeRequiredDeviceFeatures();

} // namespace trivial::rhi::vulkan

#endif // TRIVIAL_SRC_RHI_VULKAN_PHYSICAL_DEVICE_H

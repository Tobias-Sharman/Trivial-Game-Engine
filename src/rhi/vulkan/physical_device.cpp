#include "rhi/vulkan/physical_device.h"

#include <trivial/core/assert.h>
#include <trivial/core/log.h>

#include "rhi/vulkan/result.h"

namespace {

std::vector<VkQueueFamilyProperties> enumerateQueueFamilies(VkPhysicalDevice physicalDevice) {
	TRIVIAL_ASSERT(physicalDevice != VK_NULL_HANDLE);

	std::uint32_t queueFamilyCount = 0;

	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	TRIVIAL_ASSERT(queueFamilyCount > 0);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);

	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	return queueFamilies;
}

bool hasQueueFamilyPresentSupport(VkPhysicalDevice physicalDevice, std::uint32_t queueFamily, VkSurfaceKHR surface) {
	TRIVIAL_ASSERT(physicalDevice != VK_NULL_HANDLE);
	TRIVIAL_ASSERT(surface != VK_NULL_HANDLE);

	VkBool32 supportsPresent = VK_FALSE;

	const VkResult kResult
	    = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamily, surface, &supportsPresent);

	if (kResult != VK_SUCCESS) {
		TRIVIAL_LOG_ERROR("vkGetPhysicalDeviceSurfaceSupportKHR failed");
		TRIVIAL_LOG_ERROR(trivial::rhi::vulkan::resultName(kResult));
	}

	TRIVIAL_ASSERT(kResult == VK_SUCCESS);

	return supportsPresent == VK_TRUE;
}

bool supportsRequiredDeviceExtensions(std::span<const VkExtensionProperties> availableExtensions) {
	return trivial::rhi::vulkan::hasDeviceExtension(availableExtensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

// TODO: More rigourous selection once I understand how I want my queues split
trivial::rhi::vulkan::QueueFamilySelection selectQueueFamilies(VkPhysicalDevice physicalDevice,
                                                               std::span<const VkQueueFamilyProperties> queueFamilies,
                                                               VkSurfaceKHR surface) {
	trivial::rhi::vulkan::QueueFamilySelection selection = {};

	for (std::uint32_t index = 0; index < queueFamilies.size(); ++index) {
		const VkQueueFamilyProperties& queueFamily = queueFamilies[index];

		if (queueFamily.queueCount == 0) {
			continue;
		}

		const bool kSupportsGraphics = (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
		const bool kSupportsCompute = (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
		const bool kSupportsTransfer = (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0;
		const bool kSupportsPresent = hasQueueFamilyPresentSupport(physicalDevice, index, surface);

		if (kSupportsGraphics && !selection.hasGraphicsFamily) {
			selection.graphicsFamily = index;
			selection.hasGraphicsFamily = true;
		}

		if (kSupportsCompute && !selection.hasComputeFamily) {
			selection.computeFamily = index;
			selection.hasComputeFamily = true;
		}

		if (kSupportsTransfer && !selection.hasTransferFamily) {
			selection.transferFamily = index;
			selection.hasTransferFamily = true;
		}

		if (kSupportsPresent && !selection.hasPresentFamily) {
			selection.presentFamily = index;
			selection.hasPresentFamily = true;
		}
	}

	return selection;
}

void initialiseDeviceFeatures(trivial::rhi::vulkan::DeviceFeatures* features) {
	TRIVIAL_ASSERT(features != nullptr);

	*features = {};

	features->features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	features->features.pNext = &features->vulkan13;

	features->vulkan13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
}

trivial::rhi::vulkan::DeviceFeatures queryDeviceFeatures(VkPhysicalDevice physicalDevice) {
	TRIVIAL_ASSERT(physicalDevice != VK_NULL_HANDLE);

	trivial::rhi::vulkan::DeviceFeatures features = {};
	initialiseDeviceFeatures(&features);

	vkGetPhysicalDeviceFeatures2(physicalDevice, &features.features);

	return features;
}

bool supportsRequiredVulkan13Features(const VkPhysicalDeviceVulkan13Features* supportedFeatures,
                                      const VkPhysicalDeviceVulkan13Features* requiredFeatures) {
	TRIVIAL_ASSERT(supportedFeatures != nullptr);
	TRIVIAL_ASSERT(requiredFeatures != nullptr);

	// TODO: Expand for all options
	if (requiredFeatures->synchronization2 == VK_TRUE && supportedFeatures->synchronization2 != VK_TRUE) {
		return false;
	}
	if (requiredFeatures->dynamicRendering == VK_TRUE && supportedFeatures->dynamicRendering != VK_TRUE) {
		return false;
	}

	return true;
}

bool supportsRequiredDeviceFeatures(const trivial::rhi::vulkan::DeviceFeatures* supportedFeatures,
                                    const trivial::rhi::vulkan::DeviceFeatures* requiredFeatures) {
	TRIVIAL_ASSERT(supportedFeatures != nullptr);
	TRIVIAL_ASSERT(requiredFeatures != nullptr);

	return supportsRequiredVulkan13Features(&supportedFeatures->vulkan13, &requiredFeatures->vulkan13);
}

} // namespace

namespace trivial::rhi::vulkan {

std::vector<VkPhysicalDevice> enumeratePhysicalDevices(VkInstance instance) {
	TRIVIAL_ASSERT(instance != VK_NULL_HANDLE);

	std::uint32_t physicalDeviceCount = 0;

	VkResult result = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);

	if (result != VK_SUCCESS) {
		TRIVIAL_LOG_ERROR("vkEnumeratePhysicalDevices failed");
		TRIVIAL_LOG_ERROR(resultName(result));
	}

	TRIVIAL_ASSERT(result == VK_SUCCESS);
	TRIVIAL_ASSERT(physicalDeviceCount > 0);

	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);

	result = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

	if (result != VK_SUCCESS) {
		TRIVIAL_LOG_ERROR("vkEnumeratePhysicalDevices failed");
		TRIVIAL_LOG_ERROR(resultName(result));
	}

	TRIVIAL_ASSERT(result == VK_SUCCESS);

	return physicalDevices;
}

bool hasDeviceExtension(std::span<const VkExtensionProperties> availableExtensions, const char* extensionName) {
	TRIVIAL_ASSERT(extensionName != nullptr);

	for (const VkExtensionProperties& availableExtension : availableExtensions) {
		if (std::strcmp(availableExtension.extensionName, extensionName) == 0) {
			return true;
		}
	}

	return false;
}

std::vector<VkExtensionProperties> enumerateDeviceExtensions(VkPhysicalDevice physicalDevice) {
	TRIVIAL_ASSERT(physicalDevice != VK_NULL_HANDLE);

	std::uint32_t extensionCount = 0;

	VkResult result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

	if (result != VK_SUCCESS) {
		TRIVIAL_LOG_ERROR("vkEnumerateDeviceExtensionProperties failed");
		TRIVIAL_LOG_ERROR(resultName(result));
	}

	TRIVIAL_ASSERT(result == VK_SUCCESS);

	std::vector<VkExtensionProperties> extensions(extensionCount);

	if (extensionCount == 0) {
		return extensions;
	}

	result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensions.data());

	if (result != VK_SUCCESS) {
		TRIVIAL_LOG_ERROR("vkEnumerateDeviceExtensionProperties failed");
		TRIVIAL_LOG_ERROR(resultName(result));
	}

	TRIVIAL_ASSERT(result == VK_SUCCESS);

	return extensions;
}

DeviceFeatures makeRequiredDeviceFeatures() {
	DeviceFeatures features = {};
	initialiseDeviceFeatures(&features);

	features.vulkan13.synchronization2 = VK_TRUE;
	features.vulkan13.dynamicRendering = VK_TRUE;

	return features;
}

// TODO: Later add a proper selection for the best physicalDevice available and maybe allow for user switching and setting this
PhysicalDeviceSelection selectPhysicalDevice(std::span<const VkPhysicalDevice> physicalDevices, VkSurfaceKHR surface) {
	TRIVIAL_ASSERT(surface != VK_NULL_HANDLE);

	const DeviceFeatures kRequiredFeatures = makeRequiredDeviceFeatures();

	PhysicalDeviceSelection selection = {};

	for (VkPhysicalDevice physicalDevice : physicalDevices) {
		const DeviceFeatures kSupportedFeatures = queryDeviceFeatures(physicalDevice);

		if (!supportsRequiredDeviceFeatures(&kSupportedFeatures, &kRequiredFeatures)) {
			continue;
		}

		const std::vector<VkExtensionProperties> kAvailableExtensions = enumerateDeviceExtensions(physicalDevice);

		if (!supportsRequiredDeviceExtensions(kAvailableExtensions)) {
			continue;
		}

		const std::vector<VkQueueFamilyProperties> kQueueFamilies = enumerateQueueFamilies(physicalDevice);

		const QueueFamilySelection kQueueSelection = selectQueueFamilies(physicalDevice, kQueueFamilies, surface);

		if (!kQueueSelection.hasGraphicsFamily) {
			continue;
		}

		if (!kQueueSelection.hasPresentFamily) {
			continue;
		}

		selection.physicalDevice = physicalDevice;
		selection.queueFamilies = kQueueSelection;
		break;
	}

	TRIVIAL_ASSERT(selection.physicalDevice != VK_NULL_HANDLE);
	TRIVIAL_ASSERT(selection.queueFamilies.hasGraphicsFamily);
	TRIVIAL_ASSERT(selection.queueFamilies.hasPresentFamily);

	return selection;
}

} // namespace trivial::rhi::vulkan

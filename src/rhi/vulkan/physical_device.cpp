#include "rhi/vulkan/physical_device.h"

#include <cstring>

#include <trivial/core/assert.h>
#include <trivial/core/log.h>

#include "rhi/vulkan/result.h"

namespace {

std::vector<VkQueueFamilyProperties> enumerateQueueFamilies(VkPhysicalDevice physicalDevice) noexcept {
	TRIVIAL_ASSERT(physicalDevice != VK_NULL_HANDLE);

	std::uint32_t queueFamilyCount = 0;

	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	TRIVIAL_ASSERT(queueFamilyCount > 0);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);

	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	return queueFamilies;
}

bool hasQueueFamilyPresentSupport(VkPhysicalDevice physicalDevice,
                                  std::uint32_t queueFamily,
                                  VkSurfaceKHR surface) noexcept {
	TRIVIAL_ASSERT(physicalDevice != VK_NULL_HANDLE);
	TRIVIAL_ASSERT(surface != VK_NULL_HANDLE);

	VkBool32 supportsPresent = VK_FALSE;

	const VkResult kResult
	    = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamily, surface, &supportsPresent);

	TRIVIAL_VK_CHECK("vkGetPhysicalDeviceSurfaceSupportKHR failed", kResult);

	return supportsPresent == VK_TRUE;
}

bool supportsRequiredDeviceExtensions(std::span<const VkExtensionProperties> availableExtensions) noexcept {
	return trivial::rhi::vulkan::hasDeviceExtension(availableExtensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

trivial::rhi::vulkan::QueueFamilySelection selectQueueFamilies(VkPhysicalDevice physicalDevice,
                                                               std::span<const VkQueueFamilyProperties> queueFamilies,
                                                               VkSurfaceKHR surface) noexcept {
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

void initialiseDeviceFeatures(trivial::rhi::vulkan::DeviceFeatures* features) noexcept {
	TRIVIAL_ASSERT(features != nullptr);

	*features = {};

	features->features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	features->features.pNext = &features->vulkan13;

	features->vulkan13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
}

trivial::rhi::vulkan::DeviceFeatures queryDeviceFeatures(VkPhysicalDevice physicalDevice) noexcept {
	TRIVIAL_ASSERT(physicalDevice != VK_NULL_HANDLE);

	trivial::rhi::vulkan::DeviceFeatures features = {};
	initialiseDeviceFeatures(&features);

	vkGetPhysicalDeviceFeatures2(physicalDevice, &features.features);

	return features;
}

bool supportsRequiredVulkan13Features(const VkPhysicalDeviceVulkan13Features* supportedFeatures,
                                      const VkPhysicalDeviceVulkan13Features* requiredFeatures) noexcept {
	TRIVIAL_ASSERT(supportedFeatures != nullptr);
	TRIVIAL_ASSERT(requiredFeatures != nullptr);

	// NOTE: Expand for all options as needed later when caring about more support to save complexity
	if (requiredFeatures->synchronization2 == VK_TRUE && supportedFeatures->synchronization2 != VK_TRUE) {
		return false;
	}
	if (requiredFeatures->dynamicRendering == VK_TRUE && supportedFeatures->dynamicRendering != VK_TRUE) {
		return false;
	}

	return true;
}

bool supportsRequiredDeviceFeatures(const trivial::rhi::vulkan::DeviceFeatures* supportedFeatures,
                                    const trivial::rhi::vulkan::DeviceFeatures* requiredFeatures) noexcept {
	TRIVIAL_ASSERT(supportedFeatures != nullptr);
	TRIVIAL_ASSERT(requiredFeatures != nullptr);

	return supportsRequiredVulkan13Features(&supportedFeatures->vulkan13, &requiredFeatures->vulkan13);
}

} // namespace

namespace trivial::rhi::vulkan {

std::vector<VkPhysicalDevice> enumeratePhysicalDevices(VkInstance instance) noexcept {
	TRIVIAL_ASSERT(instance != VK_NULL_HANDLE);

	std::uint32_t physicalDeviceCount = 0;

	VkResult result = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);

	TRIVIAL_VK_CHECK("vkEnumeratePhysicalDevices failed", result);
	TRIVIAL_ASSERT(physicalDeviceCount > 0);

	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);

	result = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

	TRIVIAL_VK_CHECK("vkEnumeratePhysicalDevices failed", result);

	return physicalDevices;
}

bool hasDeviceExtension(std::span<const VkExtensionProperties> availableExtensions,
                        const char* extensionName) noexcept {
	TRIVIAL_ASSERT(extensionName != nullptr);

	for (const VkExtensionProperties& availableExtension : availableExtensions) {
		if (std::strcmp(availableExtension.extensionName, extensionName) == 0) {
			return true;
		}
	}

	return false;
}

std::vector<VkExtensionProperties> enumerateDeviceExtensions(VkPhysicalDevice physicalDevice) noexcept {
	TRIVIAL_ASSERT(physicalDevice != VK_NULL_HANDLE);

	std::uint32_t extensionCount = 0;

	VkResult result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

	TRIVIAL_VK_CHECK("vkEnumerateDeviceExtensionProperties failed", result);

	std::vector<VkExtensionProperties> extensions(extensionCount);

	if (extensionCount == 0) {
		return extensions;
	}

	result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensions.data());

	TRIVIAL_VK_CHECK("vkEnumerateDeviceExtensionProperties failed", result);

	return extensions;
}

DeviceFeatures makeRequiredDeviceFeatures() noexcept {
	DeviceFeatures features = {};
	initialiseDeviceFeatures(&features);

	features.vulkan13.synchronization2 = VK_TRUE;
	features.vulkan13.dynamicRendering = VK_TRUE;

	return features;
}

PhysicalDeviceSelection selectPhysicalDevice(std::span<const VkPhysicalDevice> physicalDevices,
                                             VkSurfaceKHR surface) noexcept {
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

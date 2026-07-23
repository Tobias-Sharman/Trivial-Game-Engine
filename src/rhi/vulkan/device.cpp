#include "rhi/vulkan/device.h"

#include <trivial/core/assert.h>
#include <trivial/core/log.h>

#include "rhi/vulkan/result.h"

namespace {

constexpr const char* g_kPortabilitySubsetExtensionName = "VK_KHR_portability_subset"; // To avoid beta extensions

struct DeviceSelection {
	std::vector<const char*> extensions;
};

void requireDeviceExtension(DeviceSelection* selection,
                            std::span<const VkExtensionProperties> availableExtensions,
                            const char* extensionName) {
	TRIVIAL_ASSERT(selection != nullptr);
	TRIVIAL_ASSERT(extensionName != nullptr);

	const bool kExtensionAvailable = trivial::rhi::vulkan::hasDeviceExtension(availableExtensions, extensionName);

	if (!kExtensionAvailable) {
		TRIVIAL_LOG_ERROR("required Vulkan device extension is not available");
		TRIVIAL_LOG_ERROR(extensionName);
	}

	TRIVIAL_ASSERT(kExtensionAvailable);

	selection->extensions.push_back(extensionName);
}

bool enableOptionalDeviceExtension(DeviceSelection* selection,
                                   std::span<const VkExtensionProperties> availableExtensions,
                                   const char* extensionName) {
	TRIVIAL_ASSERT(selection != nullptr);
	TRIVIAL_ASSERT(extensionName != nullptr);

	if (!trivial::rhi::vulkan::hasDeviceExtension(availableExtensions, extensionName)) {
		return false;
	}

	selection->extensions.push_back(extensionName);
	return true;
}

DeviceSelection makeDeviceSelection(std::span<const VkExtensionProperties> availableExtensions) {
	DeviceSelection selection = {};

	requireDeviceExtension(&selection, availableExtensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	(void)enableOptionalDeviceExtension(&selection, availableExtensions, g_kPortabilitySubsetExtensionName);

	return selection;
}

VkDeviceQueueCreateInfo makeDeviceQueueCreateInfo(std::uint32_t queueFamily) {
	static constexpr float s_kQueuePriority = 1.0F;

	VkDeviceQueueCreateInfo createInfo = {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
	                                      .queueFamilyIndex = queueFamily,
	                                      .queueCount = 1, // TODO: Will want to change this later
	                                      .pQueuePriorities = &s_kQueuePriority};

	return createInfo;
}

VkDeviceCreateInfo makeDeviceCreateInfo(const VkDeviceQueueCreateInfo* queueCreateInfo,
                                        const trivial::rhi::vulkan::DeviceFeatures* requiredFeatures,
                                        const DeviceSelection* selection) {
	TRIVIAL_ASSERT(queueCreateInfo != nullptr);
	TRIVIAL_ASSERT(requiredFeatures != nullptr);
	TRIVIAL_ASSERT(selection != nullptr);

	VkDeviceCreateInfo createInfo = {.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
	                                 .pNext = &requiredFeatures->vulkan13,
	                                 .queueCreateInfoCount = 1,
	                                 .pQueueCreateInfos = queueCreateInfo,
	                                 .enabledExtensionCount = static_cast<std::uint32_t>(selection->extensions.size()),
	                                 .ppEnabledExtensionNames = selection->extensions.data()};

	return createInfo;
}

} // namespace

namespace trivial::rhi::vulkan {

// TODO: change later to support just compute gpu
VkDevice createDevice(VkPhysicalDevice physicalDevice, const QueueFamilySelection* queueFamilies) {
	TRIVIAL_ASSERT(physicalDevice != VK_NULL_HANDLE);
	TRIVIAL_ASSERT(queueFamilies != nullptr);
	TRIVIAL_ASSERT(queueFamilies->hasGraphicsFamily);
	TRIVIAL_ASSERT(queueFamilies->hasPresentFamily);
	TRIVIAL_ASSERT(queueFamilies->graphicsFamily == queueFamilies->presentFamily);

	const DeviceFeatures kRequiredFeatures = makeRequiredDeviceFeatures();

	const VkDeviceQueueCreateInfo kQueueCreateInfo = makeDeviceQueueCreateInfo(queueFamilies->graphicsFamily);

	const std::vector<VkExtensionProperties> kAvailableExtensions = enumerateDeviceExtensions(physicalDevice);
	const DeviceSelection kSelection = makeDeviceSelection(kAvailableExtensions);

	const VkDeviceCreateInfo kCreateInfo = makeDeviceCreateInfo(&kQueueCreateInfo, &kRequiredFeatures, &kSelection);

	VkDevice device = VK_NULL_HANDLE;

	const VkResult kResult = vkCreateDevice(physicalDevice, &kCreateInfo, nullptr, &device);

	if (kResult != VK_SUCCESS) {
		TRIVIAL_LOG_ERROR("vkCreateDevice failed");
		TRIVIAL_LOG_ERROR(resultName(kResult));
	}

	TRIVIAL_ASSERT(kResult == VK_SUCCESS);
	TRIVIAL_ASSERT(device != VK_NULL_HANDLE);

	return device;
}

VkQueue getDeviceQueue(VkDevice device, std::uint32_t queueFamily) {
	TRIVIAL_ASSERT(device != VK_NULL_HANDLE);

	VkQueue queue = VK_NULL_HANDLE;

	vkGetDeviceQueue(device, queueFamily, 0, &queue); // TODO: Migrate from queueIndex of 0

	TRIVIAL_ASSERT(queue != VK_NULL_HANDLE);

	return queue;
}

} // namespace trivial::rhi::vulkan

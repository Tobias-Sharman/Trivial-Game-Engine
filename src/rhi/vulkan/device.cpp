#include "rhi/vulkan/device.h"

#include <span>
#include <vector>

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
                            const char* extensionName) noexcept {
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
                                   const char* extensionName) noexcept {
	TRIVIAL_ASSERT(selection != nullptr);
	TRIVIAL_ASSERT(extensionName != nullptr);

	if (!trivial::rhi::vulkan::hasDeviceExtension(availableExtensions, extensionName)) {
		return false;
	}

	selection->extensions.push_back(extensionName);
	return true;
}

DeviceSelection makeDeviceSelection(std::span<const VkExtensionProperties> availableExtensions) noexcept {
	DeviceSelection selection = {};

	requireDeviceExtension(&selection, availableExtensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	(void)enableOptionalDeviceExtension(&selection, availableExtensions, g_kPortabilitySubsetExtensionName);

	return selection;
}

VkDeviceQueueCreateInfo makeDeviceQueueCreateInfo(std::uint32_t queueFamily) noexcept {
	static constexpr float s_kQueuePriority = 1.0F;

	VkDeviceQueueCreateInfo createInfo
	    = {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
	       .pNext = nullptr,
	       .flags = 0,
	       .queueFamilyIndex = queueFamily,
	       .queueCount = 1, // Multiple queues -> mutex anyway so just use my own later as needed
	       .pQueuePriorities = &s_kQueuePriority};

	return createInfo;
}

std::vector<VkDeviceQueueCreateInfo> makeDeviceQueueCreateInfos(
    const trivial::rhi::vulkan::QueueFamilySelection* queueFamilies) noexcept {
	TRIVIAL_ASSERT(queueFamilies != nullptr);
	TRIVIAL_ASSERT(queueFamilies->hasGraphicsFamily);
	TRIVIAL_ASSERT(queueFamilies->hasPresentFamily);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

	queueCreateInfos.push_back(makeDeviceQueueCreateInfo(queueFamilies->graphicsFamily));

	if (queueFamilies->presentFamily != queueFamilies->graphicsFamily) {
		queueCreateInfos.push_back(makeDeviceQueueCreateInfo(queueFamilies->presentFamily));
	}

	return queueCreateInfos;
}

VkDeviceCreateInfo makeDeviceCreateInfo(std::span<const VkDeviceQueueCreateInfo> queueCreateInfos,
                                        const trivial::rhi::vulkan::DeviceFeatures* requiredFeatures,
                                        const DeviceSelection* selection) noexcept {
	TRIVIAL_ASSERT(!queueCreateInfos.empty());
	TRIVIAL_ASSERT(requiredFeatures != nullptr);
	TRIVIAL_ASSERT(selection != nullptr);

	VkDeviceCreateInfo createInfo = {.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
	                                 .pNext = &requiredFeatures->vulkan13,
	                                 .flags = 0,
	                                 .queueCreateInfoCount = static_cast<std::uint32_t>(queueCreateInfos.size()),
	                                 .pQueueCreateInfos = queueCreateInfos.data(),
	                                 .enabledLayerCount = 0,
	                                 .ppEnabledLayerNames = nullptr,
	                                 .enabledExtensionCount = static_cast<std::uint32_t>(selection->extensions.size()),
	                                 .ppEnabledExtensionNames = selection->extensions.data(),
	                                 .pEnabledFeatures = nullptr};

	return createInfo;
}

} // namespace

namespace trivial::rhi::vulkan {

VkDevice createDevice(VkPhysicalDevice physicalDevice, const QueueFamilySelection* queueFamilies) noexcept {
	TRIVIAL_ASSERT(physicalDevice != VK_NULL_HANDLE);
	TRIVIAL_ASSERT(queueFamilies != nullptr);
	TRIVIAL_ASSERT(queueFamilies->hasGraphicsFamily);
	TRIVIAL_ASSERT(queueFamilies->hasPresentFamily);

	const DeviceFeatures kRequiredFeatures = makeRequiredDeviceFeatures();

	const std::vector<VkDeviceQueueCreateInfo> kQueueCreateInfos = makeDeviceQueueCreateInfos(queueFamilies);

	const std::vector<VkExtensionProperties> kAvailableExtensions = enumerateDeviceExtensions(physicalDevice);
	const DeviceSelection kSelection = makeDeviceSelection(kAvailableExtensions);

	const VkDeviceCreateInfo kCreateInfo = makeDeviceCreateInfo(kQueueCreateInfos, &kRequiredFeatures, &kSelection);

	VkDevice device = VK_NULL_HANDLE;

	const VkResult kResult = vkCreateDevice(physicalDevice, &kCreateInfo, nullptr, &device);

	TRIVIAL_VK_CHECK("vkCreateDevice failed", kResult);

	TRIVIAL_ASSERT(device != VK_NULL_HANDLE);

	return device;
}

VkQueue getDeviceQueue(VkDevice device, std::uint32_t queueFamily) noexcept {
	TRIVIAL_ASSERT(device != VK_NULL_HANDLE);

	VkQueue queue = VK_NULL_HANDLE;

	// Having a single queue so just taking hardcoded first index is fine
	vkGetDeviceQueue(device, queueFamily, 0, &queue);

	TRIVIAL_ASSERT(queue != VK_NULL_HANDLE);

	return queue;
}

} // namespace trivial::rhi::vulkan

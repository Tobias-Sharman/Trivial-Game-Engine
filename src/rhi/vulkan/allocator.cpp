#define VMA_IMPLEMENTATION
#include "rhi/vulkan/allocator.h"

#include <trivial/core/assert.h>

#include "rhi/vulkan/result.h"

namespace trivial::rhi::vulkan {

VmaAllocator createAllocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device) noexcept {
	TRIVIAL_ASSERT(instance != VK_NULL_HANDLE);
	TRIVIAL_ASSERT(physicalDevice != VK_NULL_HANDLE);
	TRIVIAL_ASSERT(device != VK_NULL_HANDLE);

	static constexpr VmaVulkanFunctions s_kVulkanFunctions
	    = {.vkGetInstanceProcAddr = vkGetInstanceProcAddr, .vkGetDeviceProcAddr = vkGetDeviceProcAddr};

	const VmaAllocatorCreateInfo kAllocatorCreateInfo = {.flags = 0,
	                                                     .physicalDevice = physicalDevice,
	                                                     .device = device,
	                                                     .preferredLargeHeapBlockSize = 0,
	                                                     .pAllocationCallbacks = nullptr,
	                                                     .pDeviceMemoryCallbacks = nullptr,
	                                                     .pHeapSizeLimit = nullptr,
	                                                     .pVulkanFunctions = &s_kVulkanFunctions,
	                                                     .instance = instance,
	                                                     .vulkanApiVersion = VK_API_VERSION_1_3};

	VmaAllocator allocator = VK_NULL_HANDLE;

	const VkResult kResult = vmaCreateAllocator(&kAllocatorCreateInfo, &allocator);

	TRIVIAL_VK_CHECK("vmaCreateAllocator failed", kResult);
	TRIVIAL_ASSERT(allocator != VK_NULL_HANDLE);

	return allocator;
}

void destroyAllocator(VmaAllocator allocator) noexcept {
	if (allocator != VK_NULL_HANDLE) {
		vmaDestroyAllocator(allocator);
	}
}

} // namespace trivial::rhi::vulkan

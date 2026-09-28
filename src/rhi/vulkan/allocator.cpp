#define VMA_IMPLEMENTATION
#include "rhi/vulkan/allocator.h"

#include <vulkan/vulkan_core.h>

#include <trivial/core/assert.h>

#include "rhi/vulkan/result.h"

namespace {

constexpr VmaVulkanFunctions makeVulkanFunctions() noexcept {
	VmaVulkanFunctions functions = {};
	functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

	return functions;
}

} // namespace

namespace trivial::rhi::vulkan {

VmaAllocator createAllocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device) noexcept {
	TRIVIAL_ASSERT(instance != VK_NULL_HANDLE);
	TRIVIAL_ASSERT(physicalDevice != VK_NULL_HANDLE);
	TRIVIAL_ASSERT(device != VK_NULL_HANDLE);

	static constexpr VmaVulkanFunctions s_kVulkanFunctions = makeVulkanFunctions();

	const VmaAllocatorCreateInfo kAllocatorCreateInfo = {
	    .flags = 0,
	    .physicalDevice = physicalDevice,
	    .device = device,
	    .preferredLargeHeapBlockSize = 0,
	    .pAllocationCallbacks = nullptr,
	    .pDeviceMemoryCallbacks = nullptr,
	    .pHeapSizeLimit = nullptr,
	    .pVulkanFunctions = &s_kVulkanFunctions,
	    .instance = instance,
	    .vulkanApiVersion = VK_API_VERSION_1_3,
#if VMA_EXTERNAL_MEMORY
	    .pTypeExternalMemoryHandleTypes = nullptr,
#endif // VMA_EXTERNAL_MEMORY
	};

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

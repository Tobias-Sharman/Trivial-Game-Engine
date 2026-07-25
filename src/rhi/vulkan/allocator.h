#ifndef TRIVIAL_SRC_RHI_VULKAN_ALLOCATOR_H
#define TRIVIAL_SRC_RHI_VULKAN_ALLOCATOR_H

#include <vk_mem_alloc.h>

#include <vulkan/vulkan.h>

namespace trivial::rhi::vulkan {

VmaAllocator createAllocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device) noexcept;
void destroyAllocator(VmaAllocator allocator) noexcept;

} // namespace trivial::rhi::vulkan

#endif // TRIVIAL_SRC_RHI_VULKAN_ALLOCATOR_H

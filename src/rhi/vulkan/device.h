#ifndef TRIVIAL_SRC_RHI_VULKAN_DEVICE_H
#define TRIVIAL_SRC_RHI_VULKAN_DEVICE_H

#include <vulkan/vulkan.h>

#include "rhi/vulkan/physical_device.h"

namespace trivial::rhi::vulkan {

VkDevice createDevice(VkPhysicalDevice physicalDevice, const QueueFamilySelection* queueFamilies);
VkQueue getDeviceQueue(VkDevice device, std::uint32_t queueFamily);

} // namespace trivial::rhi::vulkan

#endif // TRIVIAL_SRC_RHI_VULKAN_DEVICE_H

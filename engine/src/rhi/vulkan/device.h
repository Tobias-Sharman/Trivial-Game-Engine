#ifndef TRIVIAL_SRC_RHI_VULKAN_DEVICE_H
#define TRIVIAL_SRC_RHI_VULKAN_DEVICE_H

#include <cstdint>

#include <vulkan/vulkan_core.h>

#include "rhi/vulkan/physical_device.h"

namespace trivial::rhi::vulkan {

[[nodiscard]] VkDevice createDevice(VkPhysicalDevice physicalDevice,
                                    const QueueFamilySelection& queueFamilies) noexcept;
[[nodiscard]] VkQueue getDeviceQueue(VkDevice device, std::uint32_t queueFamily) noexcept;

} // namespace trivial::rhi::vulkan

#endif // TRIVIAL_SRC_RHI_VULKAN_DEVICE_H

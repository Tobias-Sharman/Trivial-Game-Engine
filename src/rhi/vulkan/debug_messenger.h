#ifndef TRIVIAL_SRC_RHI_VULKAN_DEBUG_MESSENGER_H
#define TRIVIAL_SRC_RHI_VULKAN_DEBUG_MESSENGER_H

#include <trivial/core/config.h>

#if TRIVIAL_ENABLE_VULKAN_VALIDATION

#include <vulkan/vulkan_core.h>

#define TRIVIAL_RHI_VULKAN_VALIDATION_LAYER_NAME "VK_LAYER_KHRONOS_validation"

namespace trivial::rhi::vulkan {

VkDebugUtilsMessengerEXT createDebugMessenger(VkInstance instance) noexcept;
void destroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger) noexcept;

} // namespace trivial::rhi::vulkan

#endif // TRIVIAL_ENABLE_VULKAN_VALIDATION

#endif // TRIVIAL_SRC_RHI_VULKAN_DEBUG_MESSENGER_H

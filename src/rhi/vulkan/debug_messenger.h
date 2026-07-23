#ifndef TRIVIAL_SRC_RHI_VULKAN_DEBUG_MESSENGER_H
#define TRIVIAL_SRC_RHI_VULKAN_DEBUG_MESSENGER_H

#include <vulkan/vulkan.h>

#if TRIVIAL_ENABLE_VULKAN_VALIDATION
inline constexpr const char* g_kValidationLayerName = "VK_LAYER_KHRONOS_validation";
#endif

namespace trivial::rhi::vulkan {

VkDebugUtilsMessengerEXT createDebugMessenger(VkInstance instance) noexcept;
void destroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger) noexcept;

} // namespace trivial::rhi::vulkan

#endif // TRIVIAL_SRC_RHI_VULKAN_DEBUG_MESSENGER_H

#ifndef TRIVIAL_SRC_RHI_VULKAN_DEBUG_MESSENGER_H
#define TRIVIAL_SRC_RHI_VULKAN_DEBUG_MESSENGER_H

#include <vulkan/vulkan.h>

namespace trivial::rhi::vulkan {

VkDebugUtilsMessengerEXT createDebugMessenger(VkInstance instance);
void destroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger);

} // namespace trivial::rhi::vulkan

#endif // TRIVIAL_SRC_RHI_VULKAN_DEBUG_MESSENGER_H

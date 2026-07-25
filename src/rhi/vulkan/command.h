#ifndef TRIVIAL_SRC_RHI_VULKAN_COMMANDS_H
#define TRIVIAL_SRC_RHI_VULKAN_COMMANDS_H

#include <cstdint>
#include <vector>

#include <vulkan/vulkan.h>

namespace trivial::rhi::vulkan {

struct CommandState {
	VkCommandPool commandPool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> commandBuffers;
};

CommandState createCommandState(VkDevice device, std::uint32_t graphicsFamily, std::uint32_t bufferCount) noexcept;
void destroyCommandState(VkDevice device, CommandState* state) noexcept;

} // namespace trivial::rhi::vulkan

#endif // TRIVIAL_SRC_RHI_VULKAN_COMMANDS_H

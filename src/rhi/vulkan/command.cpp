#include "rhi/vulkan/command.h"

#include <trivial/core/assert.h>

#include "rhi/vulkan/result.h"

namespace trivial::rhi::vulkan {

CommandState createCommandState(VkDevice device, std::uint32_t graphicsFamily, std::uint32_t bufferCount) noexcept {
	TRIVIAL_ASSERT(device != VK_NULL_HANDLE);
	TRIVIAL_ASSERT(bufferCount > 0);

	CommandState state = {};

	const VkCommandPoolCreateInfo kPoolCreateInfo = {.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
	                                                 .pNext = nullptr,
	                                                 .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
	                                                 .queueFamilyIndex = graphicsFamily};

	const VkResult kPoolResult = vkCreateCommandPool(device, &kPoolCreateInfo, nullptr, &state.commandPool);

	TRIVIAL_VK_CHECK("vkCreateCommandPool failed", kPoolResult);
	TRIVIAL_ASSERT(state.commandPool != VK_NULL_HANDLE);

	state.commandBuffers.resize(bufferCount);

	const VkCommandBufferAllocateInfo kAllocateInfo = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
	                                                   .pNext = nullptr,
	                                                   .commandPool = state.commandPool,
	                                                   .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
	                                                   .commandBufferCount = bufferCount};

	const VkResult kAllocateResult = vkAllocateCommandBuffers(device, &kAllocateInfo, state.commandBuffers.data());

	TRIVIAL_VK_CHECK("vkAllocateCommandBuffers failed", kAllocateResult);

	return state;
}

void destroyCommandState(VkDevice device, CommandState* state) noexcept {
	TRIVIAL_ASSERT(device != VK_NULL_HANDLE);
	TRIVIAL_ASSERT(state != nullptr);

	state->commandBuffers.clear();

	if (state->commandPool != VK_NULL_HANDLE) {
		vkDestroyCommandPool(device, state->commandPool, nullptr);
		state->commandPool = VK_NULL_HANDLE;
	}
}

} // namespace trivial::rhi::vulkan

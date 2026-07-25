#include "rhi/vulkan/sync.h"

#include <trivial/core/assert.h>

#include "rhi/vulkan/result.h"

namespace {

VkSemaphore createSemaphore(VkDevice device) noexcept {
	TRIVIAL_ASSERT(device != VK_NULL_HANDLE);

	static constexpr VkSemaphoreCreateInfo s_kCreateInfo
	    = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = nullptr, .flags = 0};

	VkSemaphore semaphore = VK_NULL_HANDLE;

	const VkResult kResult = vkCreateSemaphore(device, &s_kCreateInfo, nullptr, &semaphore);

	TRIVIAL_VK_CHECK("vkCreateSemaphore failed", kResult);
	TRIVIAL_ASSERT(semaphore != VK_NULL_HANDLE);

	return semaphore;
}

VkFence createFence(VkDevice device) noexcept {
	TRIVIAL_ASSERT(device != VK_NULL_HANDLE);

	static constexpr VkFenceCreateInfo s_kCreateInfo
	    = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .pNext = nullptr, .flags = VK_FENCE_CREATE_SIGNALED_BIT};

	VkFence fence = VK_NULL_HANDLE;

	const VkResult kResult = vkCreateFence(device, &s_kCreateInfo, nullptr, &fence);

	TRIVIAL_VK_CHECK("vkCreateFence failed", kResult);
	TRIVIAL_ASSERT(fence != VK_NULL_HANDLE);

	return fence;
}

} // namespace

namespace trivial::rhi::vulkan {

FrameSyncState createFrameSyncState(VkDevice device, std::uint32_t imageCount) noexcept {
	TRIVIAL_ASSERT(device != VK_NULL_HANDLE);
	TRIVIAL_ASSERT(imageCount > 0);

	FrameSyncState state = {};

	state.imageAvailableSemaphores.reserve(imageCount);
	state.renderFinishedSemaphores.reserve(imageCount);
	state.inFlightFences.reserve(imageCount);

	for (std::uint32_t index = 0; index < imageCount; ++index) {
		state.imageAvailableSemaphores.push_back(createSemaphore(device));
		state.renderFinishedSemaphores.push_back(createSemaphore(device));
		state.inFlightFences.push_back(createFence(device));
	}

	return state;
}

void destroyFrameSyncState(VkDevice device, FrameSyncState* state) noexcept {
	TRIVIAL_ASSERT(device != VK_NULL_HANDLE);
	TRIVIAL_ASSERT(state != nullptr);

	for (VkFence fence : state->inFlightFences) {
		vkDestroyFence(device, fence, nullptr);
	}
	state->inFlightFences.clear();

	for (VkSemaphore semaphore : state->renderFinishedSemaphores) {
		vkDestroySemaphore(device, semaphore, nullptr);
	}
	state->renderFinishedSemaphores.clear();

	for (VkSemaphore semaphore : state->imageAvailableSemaphores) {
		vkDestroySemaphore(device, semaphore, nullptr);
	}
	state->imageAvailableSemaphores.clear();
}

} // namespace trivial::rhi::vulkan

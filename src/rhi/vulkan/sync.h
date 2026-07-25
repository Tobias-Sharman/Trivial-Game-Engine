#ifndef TRIVIAL_SRC_RHI_VULKAN_SYNC_H
#define TRIVIAL_SRC_RHI_VULKAN_SYNC_H

#include <cstdint>
#include <vector>

#include <vulkan/vulkan.h>

namespace trivial::rhi::vulkan {

struct FrameSyncState {
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
};

FrameSyncState createFrameSyncState(VkDevice device, std::uint32_t imageCount) noexcept;
void destroyFrameSyncState(VkDevice device, FrameSyncState* state) noexcept;

} // namespace trivial::rhi::vulkan

#endif // TRIVIAL_SRC_RHI_VULKAN_SYNC_H

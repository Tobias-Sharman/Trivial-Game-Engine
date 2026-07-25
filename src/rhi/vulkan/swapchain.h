#ifndef TRIVIAL_SRC_RHI_VULKAN_SWAPCHAIN_H
#define TRIVIAL_SRC_RHI_VULKAN_SWAPCHAIN_H

#include <cstdint>
#include <vector>

#include <vulkan/vulkan.h>

#include <trivial/engine_config.h> // TODO: Move window size out of engine config so no need to drag it all in

namespace trivial::rhi::vulkan {

struct SwapchainState {
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	std::vector<VkImage> images;
	std::vector<VkImageView> imageViews;
	VkFormat imageFormat = VK_FORMAT_UNDEFINED;
	VkExtent2D imageExtent = {};
};

struct SwapchainCreateParams {
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	WindowSize requestedSize = {.height = 0U, .width = 0U};
	std::uint32_t graphicsFamily = 0;
	std::uint32_t presentFamily = 0;
	VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE;
};

SwapchainState createSwapchain(const SwapchainCreateParams& params) noexcept;
void destroySwapchain(VkDevice device, SwapchainState* state) noexcept;

} // namespace trivial::rhi::vulkan

#endif // TRIVIAL_SRC_RHI_VULKAN_SWAPCHAIN_H

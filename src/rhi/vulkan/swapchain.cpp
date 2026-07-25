#include "rhi/vulkan/swapchain.h"

#include <span>

#include <trivial/core/assert.h>

#include "rhi/vulkan/result.h"

namespace {

VkSurfaceCapabilitiesKHR querySurfaceCapabilities(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) noexcept {
	TRIVIAL_ASSERT(physicalDevice != VK_NULL_HANDLE);
	TRIVIAL_ASSERT(surface != VK_NULL_HANDLE);

	VkSurfaceCapabilitiesKHR capabilities = {};

	const VkResult kResult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);

	TRIVIAL_VK_CHECK("vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed", kResult);

	return capabilities;
}

std::vector<VkSurfaceFormatKHR> enumerateSurfaceFormats(VkPhysicalDevice physicalDevice,
                                                        VkSurfaceKHR surface) noexcept {
	TRIVIAL_ASSERT(physicalDevice != VK_NULL_HANDLE);
	TRIVIAL_ASSERT(surface != VK_NULL_HANDLE);

	std::uint32_t formatCount = 0;

	VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

	TRIVIAL_VK_CHECK("vkGetPhysicalDeviceSurfaceFormatsKHR failed", result);
	TRIVIAL_ASSERT(formatCount > 0);

	std::vector<VkSurfaceFormatKHR> formats(formatCount);

	result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());

	TRIVIAL_VK_CHECK("vkGetPhysicalDeviceSurfaceFormatsKHR failed", result);

	return formats;
}

VkSurfaceFormatKHR selectSurfaceFormat(std::span<const VkSurfaceFormatKHR> availableFormats) noexcept {
	TRIVIAL_ASSERT(!availableFormats.empty());

	for (const VkSurfaceFormatKHR& availableFormat : availableFormats) {
		const bool kIsPreferredFormat = availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB;
		const bool kIsPreferredColorSpace = availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

		if (kIsPreferredFormat && kIsPreferredColorSpace) {
			return availableFormat;
		}
	}

	return availableFormats[0];
}

std::vector<VkPresentModeKHR> enumeratePresentModes(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) noexcept {
	TRIVIAL_ASSERT(physicalDevice != VK_NULL_HANDLE);
	TRIVIAL_ASSERT(surface != VK_NULL_HANDLE);

	std::uint32_t presentModeCount = 0;

	VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

	TRIVIAL_VK_CHECK("vkGetPhysicalDeviceSurfacePresentModesKHR failed", result);
	TRIVIAL_ASSERT(presentModeCount > 0);

	std::vector<VkPresentModeKHR> presentModes(presentModeCount);

	result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());

	TRIVIAL_VK_CHECK("vkGetPhysicalDeviceSurfacePresentModesKHR failed", result);

	return presentModes;
}

VkPresentModeKHR selectPresentMode(std::span<const VkPresentModeKHR> availablePresentModes) noexcept {
	TRIVIAL_ASSERT(!availablePresentModes.empty());

	(void)availablePresentModes;
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D selectSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, trivial::WindowSize requestedSize) noexcept {
	constexpr std::uint32_t kSpecialExtentValue = 0xFFFFFFFFU;

	// For non-sentinal value use the given size, for stuff like Wayland use the requested size within the limits of the
	// surface
	if (capabilities.currentExtent.width != kSpecialExtentValue) {
		return capabilities.currentExtent;
	}

	VkExtent2D extent = {.width = requestedSize.width, .height = requestedSize.height};

	extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

	return extent;
}

std::uint32_t selectImageCount(const VkSurfaceCapabilitiesKHR& capabilities) noexcept {
	std::uint32_t imageCount = capabilities.minImageCount + 1;

	// maxImageCount == 0 means no upper limit
	if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
		imageCount = capabilities.maxImageCount;
	}

	return imageCount;
}

std::vector<VkImage> retrieveSwapchainImages(VkDevice device, VkSwapchainKHR swapchain) noexcept {
	TRIVIAL_ASSERT(device != VK_NULL_HANDLE);
	TRIVIAL_ASSERT(swapchain != VK_NULL_HANDLE);

	std::uint32_t imageCount = 0;

	VkResult result = vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);

	TRIVIAL_VK_CHECK("vkGetSwapchainImagesKHR failed", result);
	TRIVIAL_ASSERT(imageCount > 0);

	std::vector<VkImage> images(imageCount);

	result = vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images.data());

	TRIVIAL_VK_CHECK("vkGetSwapchainImagesKHR failed", result);

	return images;
}

VkImageView createSwapchainImageView(VkDevice device, VkImage image, VkFormat format) noexcept {
	TRIVIAL_ASSERT(device != VK_NULL_HANDLE);
	TRIVIAL_ASSERT(image != VK_NULL_HANDLE);

	VkImageViewCreateInfo createInfo = {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
	                                    .pNext = nullptr,
	                                    .flags = 0,
	                                    .image = image,
	                                    .viewType = VK_IMAGE_VIEW_TYPE_2D,
	                                    .format = format,
	                                    .components = {.r = VK_COMPONENT_SWIZZLE_IDENTITY,
	                                                   .g = VK_COMPONENT_SWIZZLE_IDENTITY,
	                                                   .b = VK_COMPONENT_SWIZZLE_IDENTITY,
	                                                   .a = VK_COMPONENT_SWIZZLE_IDENTITY},
	                                    .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	                                                         .baseMipLevel = 0,
	                                                         .levelCount = 1,
	                                                         .baseArrayLayer = 0,
	                                                         .layerCount = 1}};

	VkImageView imageView = VK_NULL_HANDLE;

	const VkResult kResult = vkCreateImageView(device, &createInfo, nullptr, &imageView);

	TRIVIAL_VK_CHECK("vkCreateImageView failed", kResult);
	TRIVIAL_ASSERT(imageView != VK_NULL_HANDLE);

	return imageView;
}

} // namespace

namespace trivial::rhi::vulkan {

SwapchainState createSwapchain(const SwapchainCreateParams& params) noexcept {
	TRIVIAL_ASSERT(params.physicalDevice != VK_NULL_HANDLE);
	TRIVIAL_ASSERT(params.device != VK_NULL_HANDLE);
	TRIVIAL_ASSERT(params.surface != VK_NULL_HANDLE);

	const VkSurfaceCapabilitiesKHR kCapabilities = querySurfaceCapabilities(params.physicalDevice, params.surface);

	const std::vector<VkSurfaceFormatKHR> kAvailableFormats
	    = enumerateSurfaceFormats(params.physicalDevice, params.surface);
	const VkSurfaceFormatKHR kSurfaceFormat = selectSurfaceFormat(kAvailableFormats);

	const std::vector<VkPresentModeKHR> kAvailablePresentModes
	    = enumeratePresentModes(params.physicalDevice, params.surface);
	const VkPresentModeKHR kPresentMode = selectPresentMode(kAvailablePresentModes);

	const VkExtent2D kExtent = selectSwapExtent(kCapabilities, params.requestedSize);
	const std::uint32_t kImageCount = selectImageCount(kCapabilities);

	const bool kSameQueueFamily = params.graphicsFamily == params.presentFamily;

	const std::array<std::uint32_t, 2> kQueueFamilyIndices = {params.graphicsFamily, params.presentFamily};

	VkSwapchainCreateInfoKHR createInfo
	    = {.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
	       .pNext = nullptr,
	       .flags = 0,
	       .surface = params.surface,
	       .minImageCount = kImageCount,
	       .imageFormat = kSurfaceFormat.format,
	       .imageColorSpace = kSurfaceFormat.colorSpace,
	       .imageExtent = kExtent,
	       .imageArrayLayers = 1,
	       .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
	       .imageSharingMode = kSameQueueFamily ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
	       .queueFamilyIndexCount = kSameQueueFamily ? 0U : static_cast<std::uint32_t>(kQueueFamilyIndices.size()),
	       .pQueueFamilyIndices = kSameQueueFamily ? nullptr : kQueueFamilyIndices.data(),
	       .preTransform = kCapabilities.currentTransform,
	       .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
	       .presentMode = kPresentMode,
	       .clipped = VK_TRUE,
	       .oldSwapchain = params.oldSwapchain};

	SwapchainState state = {};
	state.imageFormat = kSurfaceFormat.format;
	state.imageExtent = kExtent;

	const VkResult kResult = vkCreateSwapchainKHR(params.device, &createInfo, nullptr, &state.swapchain);

	TRIVIAL_VK_CHECK("vkCreateSwapchainKHR failed", kResult);
	TRIVIAL_ASSERT(state.swapchain != VK_NULL_HANDLE);

	state.images = retrieveSwapchainImages(params.device, state.swapchain);

	state.imageViews.reserve(state.images.size());
	for (VkImage image : state.images) {
		state.imageViews.push_back(createSwapchainImageView(params.device, image, state.imageFormat));
	}

	return state;

	return state;
}

void destroySwapchain(VkDevice device, SwapchainState* state) noexcept {
	TRIVIAL_ASSERT(device != VK_NULL_HANDLE);
	TRIVIAL_ASSERT(state != nullptr);

	for (VkImageView imageView : state->imageViews) {
		vkDestroyImageView(device, imageView, nullptr);
	}
	state->imageViews.clear();
	state->images.clear();

	if (state->swapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(device, state->swapchain, nullptr);
		state->swapchain = VK_NULL_HANDLE;
	}

	state->imageFormat = VK_FORMAT_UNDEFINED;
	state->imageExtent = {};
}

} // namespace trivial::rhi::vulkan

#include "rhi/vulkan/debug_messenger.h"

#include <array>

#include <trivial/core/assert.h>
#include <trivial/core/log.h>

#include "rhi/vulkan/result.h"

namespace {

const char* debugMessageTypePrefix(VkDebugUtilsMessageTypeFlagsEXT messageType) noexcept {
	// NOTE: Address binding needs toggling on if wanting to use, apparently VK_EXT_device_address_binding_report
	static constexpr std::array<const char*, 16> s_kPrefixes
	    = {"[unknown] ",
	       "[general] ",
	       "[validation] ",
	       "[general validation] ",
	       "[performance] ",
	       "[general performance] ",
	       "[validation performance] ",
	       "[general validation performance] ",
	       "[device address binding] ",
	       "[general device address binding] ",
	       "[validation device address binding] ",
	       "[general validation device address binding] ",
	       "[performance device address binding] ",
	       "[general performance device address binding] ",
	       "[validation performance device address binding] ",
	       "[general validation performance device address binding] "};

	// NOTE: Will need to change if vulkan changes their style
	const std::uint32_t kIndex = static_cast<std::uint32_t>(messageType) & 0xFU;

	TRIVIAL_ASSERT(kIndex < s_kPrefixes.size());
	return s_kPrefixes[kIndex]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
}

void logDebugMessage(VkDebugUtilsMessageSeverityFlagBitsEXT severity, const char* prefix, const char* message) {
	TRIVIAL_ASSERT(prefix != nullptr);
	TRIVIAL_ASSERT(message != nullptr);

	if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		TRIVIAL_LOG_ERROR_PREFIX(prefix, message);
	} else if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		TRIVIAL_LOG_WARNING_PREFIX(prefix, message);
	} else if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
		TRIVIAL_LOG_INFO_PREFIX(prefix, message);
	} else {
		TRIVIAL_LOG_DEBUG_PREFIX(prefix, message);
	}
}
VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                             VkDebugUtilsMessageTypeFlagsEXT messageType,
                                             const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
                                             void* userData) noexcept {
	(void)userData;

	TRIVIAL_ASSERT(callbackData != nullptr);
	TRIVIAL_ASSERT(callbackData->pMessage != nullptr);

	logDebugMessage(severity, debugMessageTypePrefix(messageType), callbackData->pMessage);

	return VK_FALSE;
}

VkDebugUtilsMessengerCreateInfoEXT makeDebugMessengerCreateInfo() noexcept {
	static constexpr VkDebugUtilsMessengerCreateInfoEXT s_kCreateInfo
	    = {.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
	       .pNext = nullptr,
	       .flags = 0,
	       .messageSeverity
	       = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
	       .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
	                      | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
	       .pfnUserCallback = debugCallback,
	       .pUserData = nullptr};

	return s_kCreateInfo;
}

} // namespace

namespace trivial::rhi::vulkan {

VkDebugUtilsMessengerEXT createDebugMessenger(VkInstance instance) noexcept {
	TRIVIAL_ASSERT(instance != VK_NULL_HANDLE);

	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	const auto kCreateDebugUtilsMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
	    vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

	if (kCreateDebugUtilsMessenger == nullptr) {
		TRIVIAL_LOG_ERROR("vkCreateDebugUtilsMessengerEXT is not available");
	}

	TRIVIAL_ASSERT(kCreateDebugUtilsMessenger != nullptr);

	const VkDebugUtilsMessengerCreateInfoEXT kCreateInfo = makeDebugMessengerCreateInfo();

	VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

	const VkResult kResult = kCreateDebugUtilsMessenger(instance, &kCreateInfo, nullptr, &debugMessenger);

	TRIVIAL_VK_CHECK("vkCreateDebugUtilsMessengerEXT failed", kResult);

	TRIVIAL_ASSERT(debugMessenger != VK_NULL_HANDLE);

	return debugMessenger;
}

void destroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger) noexcept {
	TRIVIAL_ASSERT(instance != VK_NULL_HANDLE);

	if (debugMessenger == VK_NULL_HANDLE) {
		return;
	}

	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	const auto kDestroyDebugUtilsMessenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
	    vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

	TRIVIAL_ASSERT(kDestroyDebugUtilsMessenger != nullptr);

	kDestroyDebugUtilsMessenger(instance, debugMessenger, nullptr);
}

} // namespace trivial::rhi::vulkan

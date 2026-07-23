#include "rhi/vulkan/debug_messenger.h"

#include <trivial/core/assert.h>
#include <trivial/core/log.h>

#include "rhi/vulkan/result.h"

namespace {

const char* debugMessageTypePrefix(VkDebugUtilsMessageTypeFlagsEXT messageType) {
	const bool kGeneral = (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) != 0;
	const bool kValidation = (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) != 0;
	const bool kPerformance = (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) != 0;

	if (kGeneral && kValidation && kPerformance) {
		return "[general validation performance] ";
	}
	if (kGeneral && kValidation) {
		return "[general validation] ";
	}
	if (kGeneral && kPerformance) {
		return "[general performance] ";
	}
	if (kValidation && kPerformance) {
		return "[validation performance] ";
	}
	if (kGeneral) {
		return "[general] ";
	}
	if (kValidation) {
		return "[validation] ";
	}
	if (kPerformance) {
		return "[performance] ";
	}
	return "[unknown] ";
}

void logDebugMessage(VkDebugUtilsMessageSeverityFlagBitsEXT severity, const char* prefix, const char* message) {
	TRIVIAL_ASSERT(prefix != nullptr);
	TRIVIAL_ASSERT(message != nullptr);

	if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		TRIVIAL_LOG_ERROR_PREFIX(prefix, message);
	} else if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		TRIVIAL_LOG_WARNING_PREFIX(prefix, message);
	} else {
		TRIVIAL_LOG_INFO_PREFIX(prefix, message);
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                             VkDebugUtilsMessageTypeFlagsEXT messageType,
                                             const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
                                             void* userData) {
	(void)userData;

	TRIVIAL_ASSERT(callbackData != nullptr);
	TRIVIAL_ASSERT(callbackData->pMessage != nullptr);

	logDebugMessage(severity, debugMessageTypePrefix(messageType), callbackData->pMessage);

	return VK_FALSE;
}

VkDebugUtilsMessengerCreateInfoEXT makeDebugMessengerCreateInfo() {
	VkDebugUtilsMessengerCreateInfoEXT createInfo
	    = {.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
	       .messageSeverity
	       = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
	       .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
	                      | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
	       .pfnUserCallback = debugCallback,
	       .pUserData = nullptr};

	return createInfo;
}

} // namespace

namespace trivial::rhi::vulkan {

VkDebugUtilsMessengerEXT createDebugMessenger(VkInstance instance) {
	TRIVIAL_ASSERT(instance != VK_NULL_HANDLE);

	const auto kCreateDebugUtilsMessenger
	    = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>( // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
	        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

	if (kCreateDebugUtilsMessenger == nullptr) {
		TRIVIAL_LOG_ERROR("vkCreateDebugUtilsMessengerEXT is not available");
	}

	TRIVIAL_ASSERT(kCreateDebugUtilsMessenger != nullptr);

	const VkDebugUtilsMessengerCreateInfoEXT kCreateInfo = makeDebugMessengerCreateInfo();

	VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

	const VkResult kResult = kCreateDebugUtilsMessenger(instance, &kCreateInfo, nullptr, &debugMessenger);

	if (kResult != VK_SUCCESS) {
		TRIVIAL_LOG_ERROR("vkCreateDebugUtilsMessengerEXT failed");
		TRIVIAL_LOG_ERROR(resultName(kResult));
	}

	TRIVIAL_ASSERT(kResult == VK_SUCCESS);
	TRIVIAL_ASSERT(debugMessenger != VK_NULL_HANDLE);

	return debugMessenger;
}

void destroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger) {
	TRIVIAL_ASSERT(instance != VK_NULL_HANDLE);

	if (debugMessenger == VK_NULL_HANDLE) {
		return;
	}

	const auto kDestroyDebugUtilsMessenger
	    = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>( // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
	        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

	TRIVIAL_ASSERT(kDestroyDebugUtilsMessenger != nullptr);

	kDestroyDebugUtilsMessenger(instance, debugMessenger, nullptr);
}

} // namespace trivial::rhi::vulkan

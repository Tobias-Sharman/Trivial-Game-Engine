#include "rhi/vulkan/instance.h"

#include <span>
#include <vector>

#include <trivial/core/assert.h>
#include <trivial/core/config.h>
#include <trivial/core/log.h>
#include <trivial/platform/window.h>

#include "rhi/vulkan/result.h"

namespace {

constexpr const char* g_kValidationLayerName = "VK_LAYER_KHRONOS_validation";

struct InstanceSelection {
	std::vector<const char*> extensions;
	std::vector<const char*> layers;
	VkInstanceCreateFlags flags = 0;
};

std::uint32_t makeVulkanVersion(trivial::Version version) {
	TRIVIAL_ASSERT(version.major >= 0);
	TRIVIAL_ASSERT(version.minor >= 0);
	TRIVIAL_ASSERT(version.patch >= 0);

	return VK_MAKE_VERSION(static_cast<std::uint32_t>(version.major),
	                       static_cast<std::uint32_t>(version.minor),
	                       static_cast<std::uint32_t>(version.patch));
}

bool hasInstanceExtension(std::span<const VkExtensionProperties> availableExtensions, const char* extensionName) {
	TRIVIAL_ASSERT(extensionName != nullptr);

	for (const VkExtensionProperties& availableExtension : availableExtensions) {
		if (std::strcmp(availableExtension.extensionName, extensionName) == 0) {
			return true;
		}
	}

	return false;
}

bool hasInstanceLayer(std::span<const VkLayerProperties> availableLayers, const char* layerName) {
	TRIVIAL_ASSERT(layerName != nullptr);

	for (const VkLayerProperties& availableLayer : availableLayers) {
		if (std::strcmp(availableLayer.layerName, layerName) == 0) {
			return true;
		}
	}

	return false;
}

std::vector<VkExtensionProperties> enumerateInstanceExtensions() {
	std::uint32_t extensionCount = 0;

	VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	if (result != VK_SUCCESS) {
		TRIVIAL_LOG_ERROR("vkEnumerateInstanceExtensionProperties failed");
		TRIVIAL_LOG_ERROR(trivial::rhi::vulkan::resultName(result));
	}

	TRIVIAL_ASSERT(result == VK_SUCCESS);

	std::vector<VkExtensionProperties> extensions(extensionCount);

	if (extensionCount == 0) {
		return extensions;
	}

	result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	if (result != VK_SUCCESS) {
		TRIVIAL_LOG_ERROR("vkEnumerateInstanceExtensionProperties failed");
		TRIVIAL_LOG_ERROR(trivial::rhi::vulkan::resultName(result));
	}

	TRIVIAL_ASSERT(result == VK_SUCCESS);

	return extensions;
}

std::vector<VkLayerProperties> enumerateInstanceLayers() {
	std::uint32_t layerCount = 0;

	VkResult result = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	if (result != VK_SUCCESS) {
		TRIVIAL_LOG_ERROR("vkEnumerateInstanceLayerProperties failed");
		TRIVIAL_LOG_ERROR(trivial::rhi::vulkan::resultName(result));
	}

	TRIVIAL_ASSERT(result == VK_SUCCESS);

	std::vector<VkLayerProperties> layers(layerCount);

	if (layerCount == 0) {
		return layers;
	}

	result = vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

	if (result != VK_SUCCESS) {
		TRIVIAL_LOG_ERROR("vkEnumerateInstanceLayerProperties failed");
		TRIVIAL_LOG_ERROR(trivial::rhi::vulkan::resultName(result));
	}

	TRIVIAL_ASSERT(result == VK_SUCCESS);

	return layers;
}

void requireInstanceExtension(InstanceSelection* selection,
                              std::span<const VkExtensionProperties> availableExtensions,
                              const char* extensionName) {

	TRIVIAL_ASSERT(selection != nullptr);
	TRIVIAL_ASSERT(extensionName != nullptr);

	const bool kExtensionAvailable = hasInstanceExtension(availableExtensions, extensionName);
	if (!kExtensionAvailable) {
		TRIVIAL_LOG_ERROR("required Vulkan instance extension is not available");
		TRIVIAL_LOG_ERROR(extensionName);
	}

	TRIVIAL_ASSERT(kExtensionAvailable);

	selection->extensions.push_back(extensionName);
}

bool enableOptionalInstanceExtension(InstanceSelection* selection,
                                     std::span<const VkExtensionProperties> availableExtensions,
                                     const char* extensionName) {
	TRIVIAL_ASSERT(selection != nullptr);
	TRIVIAL_ASSERT(extensionName != nullptr);

	if (!hasInstanceExtension(availableExtensions, extensionName)) {
		return false;
	}

	selection->extensions.push_back(extensionName);
	return true;
}

bool enableOptionalInstanceLayer(InstanceSelection* selection,
                                 std::span<const VkLayerProperties> availableLayers,
                                 const char* layerName) {
	TRIVIAL_ASSERT(selection != nullptr);
	TRIVIAL_ASSERT(layerName != nullptr);

	if (!hasInstanceLayer(availableLayers, layerName)) {
		return false;
	}

	selection->layers.push_back(layerName);
	return true;
}

InstanceSelection makeInstanceSelection(std::span<const char* const> requiredExtensions,
                                        std::span<const VkExtensionProperties> availableExtensions,
                                        std::span<const VkLayerProperties> availableLayers) {
	TRIVIAL_ASSERT(!requiredExtensions.empty());

	InstanceSelection selection = {};

	for (const char* requiredExtension : requiredExtensions) {
		requireInstanceExtension(&selection, availableExtensions, requiredExtension);
	}

	const bool kPortabilityEnumerationEnabled
	    = enableOptionalInstanceExtension(&selection,
	                                      availableExtensions,
	                                      VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

	if (kPortabilityEnumerationEnabled) {
		selection.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
	}

#if TRIVIAL_ENABLE_VULKAN_VALIDATION
	const bool kValidationLayerEnabled
	    = enableOptionalInstanceLayer(&selection, availableLayers, g_kValidationLayerName);

	if (!kValidationLayerEnabled) {
		TRIVIAL_LOG_WARNING("VK_LAYER_KHRONOS_validation is not available");
	}

	const bool kDebugUtilsEnabled
	    = enableOptionalInstanceExtension(&selection, availableExtensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	if (!kDebugUtilsEnabled) {
		TRIVIAL_LOG_WARNING("VK_EXT_debug_utils is not available");
	}
#endif // TRIVIAL_ENABLE_VULKAN_VALIDATION

	return selection;
}

VkApplicationInfo makeApplicationInfo(const trivial::EngineConfig* config) {
	TRIVIAL_ASSERT(config != nullptr);
	TRIVIAL_ASSERT(!config->applicationName.empty());
	TRIVIAL_ASSERT(!config->engineName.empty());

	VkApplicationInfo applicationInfo = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
	                                     .pApplicationName = config->applicationName.c_str(),
	                                     .applicationVersion = makeVulkanVersion(config->applicationVersion),
	                                     .pEngineName = config->engineName.c_str(),
	                                     .engineVersion = makeVulkanVersion(config->engineVersion),
	                                     .apiVersion = VK_API_VERSION_1_3};

	return applicationInfo;
}

VkInstanceCreateInfo makeInstanceCreateInfo(const VkApplicationInfo* applicationInfo,
                                            const InstanceSelection* selection) {
	TRIVIAL_ASSERT(applicationInfo != nullptr);
	TRIVIAL_ASSERT(selection != nullptr);
	TRIVIAL_ASSERT(!selection->extensions.empty());

	VkInstanceCreateInfo createInfo
	    = {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
	       .flags = selection->flags,
	       .pApplicationInfo = applicationInfo,
	       .enabledLayerCount = static_cast<std::uint32_t>(selection->layers.size()),
	       .ppEnabledLayerNames = selection->layers.data(),
	       .enabledExtensionCount = static_cast<std::uint32_t>(selection->extensions.size()),
	       .ppEnabledExtensionNames = selection->extensions.data()};

	return createInfo;
}

} // namespace

namespace trivial::rhi::vulkan {

VkInstance createInstance(const EngineConfig* config) {
	TRIVIAL_ASSERT(config != nullptr);

	const std::span<const char* const> kRequiredExtensions
	    = trivial::platform::Window::requiredVulkanInstanceExtensions();

	TRIVIAL_ASSERT(!kRequiredExtensions.empty());

	const std::vector<VkExtensionProperties> kAvailableExtensions = enumerateInstanceExtensions();
	const std::vector<VkLayerProperties> kAvailableLayers = enumerateInstanceLayers();

	const InstanceSelection kSelection
	    = makeInstanceSelection(kRequiredExtensions, kAvailableExtensions, kAvailableLayers);

	const VkApplicationInfo kApplicationInfo = makeApplicationInfo(config);

	const VkInstanceCreateInfo kCreateInfo = makeInstanceCreateInfo(&kApplicationInfo, &kSelection);

	VkInstance instance = VK_NULL_HANDLE;

	const VkResult kResult = vkCreateInstance(&kCreateInfo, nullptr, &instance);

	if (kResult != VK_SUCCESS) {
		TRIVIAL_LOG_ERROR("vkCreateInstance failed");
		TRIVIAL_LOG_ERROR(resultName(kResult));
	}

	TRIVIAL_ASSERT(kResult == VK_SUCCESS);
	TRIVIAL_ASSERT(instance != VK_NULL_HANDLE);

	return instance;
}

} // namespace trivial::rhi::vulkan

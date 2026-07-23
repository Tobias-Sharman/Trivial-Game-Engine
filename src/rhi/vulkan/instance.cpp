#include "rhi/vulkan/instance.h"

#include <cstring>
#include <span>
#include <vector>

#include <trivial/core/assert.h>
#include <trivial/core/config.h>
#include <trivial/core/log.h>
#include <trivial/platform/window.h>

#include "rhi/vulkan/debug_messenger.h"
#include "rhi/vulkan/result.h"

namespace {

struct InstanceSelection {
	std::vector<const char*> extensions;
	std::vector<const char*> layers;
	VkInstanceCreateFlags flags = 0;
};

bool hasInstanceExtension(std::span<const VkExtensionProperties> availableExtensions,
                          const char* extensionName) noexcept {
	TRIVIAL_ASSERT(extensionName != nullptr);

	for (const VkExtensionProperties& availableExtension : availableExtensions) {
		if (std::strcmp(availableExtension.extensionName, extensionName) == 0) {
			return true;
		}
	}

	return false;
}

bool hasInstanceLayer(std::span<const VkLayerProperties> availableLayers, const char* layerName) noexcept {
	TRIVIAL_ASSERT(layerName != nullptr);

	for (const VkLayerProperties& availableLayer : availableLayers) {
		if (std::strcmp(availableLayer.layerName, layerName) == 0) {
			return true;
		}
	}

	return false;
}

std::vector<VkExtensionProperties> enumerateInstanceExtensions() noexcept {
	std::uint32_t extensionCount = 0;

	VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	TRIVIAL_VK_CHECK("vkEnumerateInstanceExtensionProperties failed", result);

	std::vector<VkExtensionProperties> extensions(extensionCount);

	if (extensionCount == 0) {
		return extensions;
	}

	result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	TRIVIAL_VK_CHECK("vkEnumerateInstanceExtensionProperties failed", result);

	return extensions;
}

std::vector<VkLayerProperties> enumerateInstanceLayers() noexcept {
	std::uint32_t layerCount = 0;

	VkResult result = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	TRIVIAL_VK_CHECK("vkEnumerateInstanceLayerProperties failed", result);

	std::vector<VkLayerProperties> layers(layerCount);

	if (layerCount == 0) {
		return layers;
	}

	result = vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

	TRIVIAL_VK_CHECK("vkEnumerateInstanceLayerProperties failed", result);

	return layers;
}

void requireInstanceExtension(InstanceSelection* selection,
                              std::span<const VkExtensionProperties> availableExtensions,
                              const char* extensionName) noexcept {

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
                                     const char* extensionName) noexcept {
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
                                 const char* layerName) noexcept {
	TRIVIAL_ASSERT(selection != nullptr);
	TRIVIAL_ASSERT(layerName != nullptr);

	if (!hasInstanceLayer(availableLayers, layerName)) {
		return false;
	}

	selection->layers.push_back(layerName);
	return true;
}

void addRequiredExtensions(InstanceSelection* selection,
                           std::span<const char* const> requiredExtensions,
                           std::span<const VkExtensionProperties> availableExtensions) noexcept {
	for (const char* requiredExtension : requiredExtensions) {
		requireInstanceExtension(selection, availableExtensions, requiredExtension);
	}
}

void enableOptionalPortability(InstanceSelection* selection,
                               std::span<const VkExtensionProperties> availableExtensions) noexcept {
	const bool kPortabilityEnumerationEnabled
	    = enableOptionalInstanceExtension(selection,
	                                      availableExtensions,
	                                      VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

	if (kPortabilityEnumerationEnabled) {
		selection->flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
	}
}

#if TRIVIAL_ENABLE_VULKAN_VALIDATION

void enableOptionalValidation(InstanceSelection* selection,
                              std::span<const VkExtensionProperties> availableExtensions,
                              std::span<const VkLayerProperties> availableLayers) noexcept {
	const bool kValidationLayerEnabled
	    = enableOptionalInstanceLayer(selection, availableLayers, g_kValidationLayerName);

	if (!kValidationLayerEnabled) {
		TRIVIAL_LOG_WARNING("VK_LAYER_KHRONOS_validation is not available");
	}

	const bool kDebugUtilsEnabled
	    = enableOptionalInstanceExtension(selection, availableExtensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	if (!kDebugUtilsEnabled) {
		TRIVIAL_LOG_WARNING("VK_EXT_debug_utils is not available");
	}
}

#endif // TRIVIAL_ENABLE_VULKAN_VALIDATION

InstanceSelection makeInstanceSelection(std::span<const char* const> requiredExtensions,
                                        std::span<const VkExtensionProperties> availableExtensions,
                                        std::span<const VkLayerProperties> availableLayers) noexcept {
	TRIVIAL_ASSERT(!requiredExtensions.empty());

	InstanceSelection selection = {};

	addRequiredExtensions(&selection, requiredExtensions, availableExtensions);
	enableOptionalPortability(&selection, availableExtensions);

#if TRIVIAL_ENABLE_VULKAN_VALIDATION
	enableOptionalValidation(&selection, availableExtensions, availableLayers);
#endif // TRIVIAL_ENABLE_VULKAN_VALIDATION

	return selection;
}

std::uint32_t makeVulkanVersion(trivial::Version version) noexcept {
	return VK_MAKE_VERSION(version.major, version.minor, version.patch);
}

VkApplicationInfo makeApplicationInfo(const trivial::EngineConfig* config) noexcept {
	TRIVIAL_ASSERT(config != nullptr);
	TRIVIAL_ASSERT(!config->applicationName.empty());
	TRIVIAL_ASSERT(!config->engineName.empty());

	VkApplicationInfo applicationInfo = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
	                                     .pNext = nullptr,
	                                     .pApplicationName = config->applicationName.c_str(),
	                                     .applicationVersion = makeVulkanVersion(config->applicationVersion),
	                                     .pEngineName = config->engineName.c_str(),
	                                     .engineVersion = makeVulkanVersion(config->engineVersion),
	                                     .apiVersion = VK_API_VERSION_1_3};

	return applicationInfo;
}

VkInstanceCreateInfo makeInstanceCreateInfo(const VkApplicationInfo* applicationInfo,
                                            const InstanceSelection* selection) noexcept {
	TRIVIAL_ASSERT(applicationInfo != nullptr);
	TRIVIAL_ASSERT(selection != nullptr);
	TRIVIAL_ASSERT(!selection->extensions.empty());

	VkInstanceCreateInfo createInfo
	    = {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
	       .pNext = nullptr,
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

VkInstance createInstance(const EngineConfig* config) noexcept {
	TRIVIAL_ASSERT(config != nullptr);

	const std::span<const char* const> kRequiredExtensions
	    = trivial::platform::Window::requiredVulkanInstanceExtensions();

	TRIVIAL_ASSERT(!kRequiredExtensions.empty());

	const std::vector<VkExtensionProperties> kAvailableExtensions = enumerateInstanceExtensions();

#if TRIVIAL_ENABLE_VULKAN_VALIDATION
	const std::vector<VkLayerProperties> kAvailableLayers = enumerateInstanceLayers();
#else
	const std::vector<VkLayerProperties> kAvailableLayers = {};
#endif

	const InstanceSelection kSelection
	    = makeInstanceSelection(kRequiredExtensions, kAvailableExtensions, kAvailableLayers);

	const VkApplicationInfo kApplicationInfo = makeApplicationInfo(config);

	const VkInstanceCreateInfo kCreateInfo = makeInstanceCreateInfo(&kApplicationInfo, &kSelection);

	VkInstance instance = VK_NULL_HANDLE;

	const VkResult kResult = vkCreateInstance(&kCreateInfo, nullptr, &instance);

	TRIVIAL_VK_CHECK("vkCreateInstance failed", kResult);

	TRIVIAL_ASSERT(instance != VK_NULL_HANDLE);

	return instance;
}

} // namespace trivial::rhi::vulkan

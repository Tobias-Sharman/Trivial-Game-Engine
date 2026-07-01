#include <trivial/internal/rhi/vulkan/backend.h>

#include <cstring>
#include <span>
#include <vector>

#include <trivial/internal/rhi/vulkan/result.h>

#include "core/assert.h"
#include "core/log.h"

namespace trivial::internal::rhi::vulkan {

namespace {

//TODO: Reorder this this is a mess

constexpr const char* g_kValidationLayerName = "VK_LAYER_KHRONOS_validation";
constexpr const char* g_kPortabilitySubsetExtensionName = "VK_KHR_portability_subset"; // To avoid beta extensions

struct InstanceSelection {
	std::vector<const char*> extensions;
	std::vector<const char*> layers;
	VkInstanceCreateFlags flags = 0;
};

struct DeviceSelection {
	std::vector<const char*> extensions;
};

struct QueueFamilySelection {
	std::uint32_t graphicsFamily = 0;
	std::uint32_t computeFamily = 0;
	std::uint32_t transferFamily = 0;
	std::uint32_t presentFamily = 0;

	bool hasGraphicsFamily = false;
	bool hasComputeFamily = false;
	bool hasTransferFamily = false;
	bool hasPresentFamily = false;
};

struct PhysicalDeviceSelection {
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	QueueFamilySelection queueFamilies = {};
};

struct DeviceFeatures {
	VkPhysicalDeviceFeatures2 features = {};
	VkPhysicalDeviceVulkan13Features vulkan13 = {};
};

std::uint32_t makeVulkanVersion(Version version) {
	TRIVIAL_ASSERT(version.major >= 0);
	TRIVIAL_ASSERT(version.minor >= 0);
	TRIVIAL_ASSERT(version.patch >= 0);

	return VK_MAKE_VERSION(static_cast<std::uint32_t>(version.major),
	                       static_cast<std::uint32_t>(version.minor),
	                       static_cast<std::uint32_t>(version.patch));
}

VkApplicationInfo makeApplicationInfo(const EngineConfig* config) {
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

std::vector<VkExtensionProperties> enumerateInstanceExtensions() {
	std::uint32_t extensionCount = 0;

	VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	if (result != VK_SUCCESS) {
		TRIVIAL_LOG_ERROR("vkEnumerateInstanceExtensionProperties failed");
		TRIVIAL_LOG_ERROR(resultName(result));
	}

	TRIVIAL_ASSERT(result == VK_SUCCESS);

	std::vector<VkExtensionProperties> extensions(extensionCount);

	if (extensionCount == 0) {
		return extensions;
	}

	result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	if (result != VK_SUCCESS) {
		TRIVIAL_LOG_ERROR("vkEnumerateInstanceExtensionProperties failed");
		TRIVIAL_LOG_ERROR(resultName(result));
	}

	TRIVIAL_ASSERT(result == VK_SUCCESS);

	return extensions;
}

std::vector<VkLayerProperties> enumerateInstanceLayers() {
	std::uint32_t layerCount = 0;

	VkResult result = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	if (result != VK_SUCCESS) {
		TRIVIAL_LOG_ERROR("vkEnumerateInstanceLayerProperties failed");
		TRIVIAL_LOG_ERROR(resultName(result));
	}

	TRIVIAL_ASSERT(result == VK_SUCCESS);

	std::vector<VkLayerProperties> layers(layerCount);

	if (layerCount == 0) {
		return layers;
	}

	result = vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

	if (result != VK_SUCCESS) {
		TRIVIAL_LOG_ERROR("vkEnumerateInstanceLayerProperties failed");
		TRIVIAL_LOG_ERROR(resultName(result));
	}

	TRIVIAL_ASSERT(result == VK_SUCCESS);

	return layers;
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

VkInstance createInstance(const EngineConfig* config) {
	TRIVIAL_ASSERT(config != nullptr);

	const std::span<const char* const> kRequiredExtensions
	    = trivial::internal::platform::Window::requiredVulkanInstanceExtensions();

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

#if TRIVIAL_ENABLE_VULKAN_VALIDATION

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

#endif // TRIVIAL_ENABLE_VULKAN_VALIDATION

std::vector<VkPhysicalDevice> enumeratePhysicalDevices(VkInstance instance) {
	TRIVIAL_ASSERT(instance != VK_NULL_HANDLE);

	std::uint32_t physicalDeviceCount = 0;

	VkResult result = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);

	if (result != VK_SUCCESS) {
		TRIVIAL_LOG_ERROR("vkEnumeratePhysicalDevices failed");
		TRIVIAL_LOG_ERROR(resultName(result));
	}

	TRIVIAL_ASSERT(result == VK_SUCCESS);
	TRIVIAL_ASSERT(physicalDeviceCount > 0);

	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);

	result = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

	if (result != VK_SUCCESS) {
		TRIVIAL_LOG_ERROR("vkEnumeratePhysicalDevices failed");
		TRIVIAL_LOG_ERROR(resultName(result));
	}

	TRIVIAL_ASSERT(result == VK_SUCCESS);

	return physicalDevices;
}

std::vector<VkExtensionProperties> enumerateDeviceExtensions(VkPhysicalDevice physicalDevice) {
	TRIVIAL_ASSERT(physicalDevice != VK_NULL_HANDLE);

	std::uint32_t extensionCount = 0;

	VkResult result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

	if (result != VK_SUCCESS) {
		TRIVIAL_LOG_ERROR("vkEnumerateDeviceExtensionProperties failed");
		TRIVIAL_LOG_ERROR(resultName(result));
	}

	TRIVIAL_ASSERT(result == VK_SUCCESS);

	std::vector<VkExtensionProperties> extensions(extensionCount);

	if (extensionCount == 0) {
		return extensions;
	}

	result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensions.data());

	if (result != VK_SUCCESS) {
		TRIVIAL_LOG_ERROR("vkEnumerateDeviceExtensionProperties failed");
		TRIVIAL_LOG_ERROR(resultName(result));
	}

	TRIVIAL_ASSERT(result == VK_SUCCESS);

	return extensions;
}

std::vector<VkQueueFamilyProperties> enumerateQueueFamilies(VkPhysicalDevice physicalDevice) {
	TRIVIAL_ASSERT(physicalDevice != VK_NULL_HANDLE);

	std::uint32_t queueFamilyCount = 0;

	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	TRIVIAL_ASSERT(queueFamilyCount > 0);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);

	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	return queueFamilies;
}

bool hasDeviceExtension(std::span<const VkExtensionProperties> availableExtensions, const char* extensionName) {
	TRIVIAL_ASSERT(extensionName != nullptr);

	for (const VkExtensionProperties& availableExtension : availableExtensions) {
		if (std::strcmp(availableExtension.extensionName, extensionName) == 0) {
			return true;
		}
	}

	return false;
}

bool hasQueueFamilyPresentSupport(VkPhysicalDevice physicalDevice, std::uint32_t queueFamily, VkSurfaceKHR surface) {
	TRIVIAL_ASSERT(physicalDevice != VK_NULL_HANDLE);
	TRIVIAL_ASSERT(surface != VK_NULL_HANDLE);

	VkBool32 supportsPresent = VK_FALSE;

	const VkResult kResult
	    = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamily, surface, &supportsPresent);

	if (kResult != VK_SUCCESS) {
		TRIVIAL_LOG_ERROR("vkGetPhysicalDeviceSurfaceSupportKHR failed");
		TRIVIAL_LOG_ERROR(resultName(kResult));
	}

	TRIVIAL_ASSERT(kResult == VK_SUCCESS);

	return supportsPresent == VK_TRUE;
}

void requireDeviceExtension(DeviceSelection* selection,
                            std::span<const VkExtensionProperties> availableExtensions,
                            const char* extensionName) {
	TRIVIAL_ASSERT(selection != nullptr);
	TRIVIAL_ASSERT(extensionName != nullptr);

	const bool kExtensionAvailable = hasDeviceExtension(availableExtensions, extensionName);

	if (!kExtensionAvailable) {
		TRIVIAL_LOG_ERROR("required Vulkan device extension is not available");
		TRIVIAL_LOG_ERROR(extensionName);
	}

	TRIVIAL_ASSERT(kExtensionAvailable);

	selection->extensions.push_back(extensionName);
}

bool supportsRequiredDeviceExtensions(std::span<const VkExtensionProperties> availableExtensions) {
	return hasDeviceExtension(availableExtensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

bool enableOptionalDeviceExtension(DeviceSelection* selection,
                                   std::span<const VkExtensionProperties> availableExtensions,
                                   const char* extensionName) {
	TRIVIAL_ASSERT(selection != nullptr);
	TRIVIAL_ASSERT(extensionName != nullptr);

	if (!hasDeviceExtension(availableExtensions, extensionName)) {
		return false;
	}

	selection->extensions.push_back(extensionName);
	return true;
}

DeviceSelection makeDeviceSelection(std::span<const VkExtensionProperties> availableExtensions) {
	DeviceSelection selection = {};

	requireDeviceExtension(&selection, availableExtensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	(void)enableOptionalDeviceExtension(&selection, availableExtensions, g_kPortabilitySubsetExtensionName);

	return selection;
}

// TODO: More rigourous selection once I understand how I want my queues split
QueueFamilySelection selectQueueFamilies(VkPhysicalDevice physicalDevice,
                                         std::span<const VkQueueFamilyProperties> queueFamilies,
                                         VkSurfaceKHR surface) {
	QueueFamilySelection selection = {};

	for (std::uint32_t index = 0; index < queueFamilies.size(); ++index) {
		const VkQueueFamilyProperties& queueFamily = queueFamilies[index];

		if (queueFamily.queueCount == 0) {
			continue;
		}

		const bool kSupportsGraphics = (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
		const bool kSupportsCompute = (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
		const bool kSupportsTransfer = (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0;
		const bool kSupportsPresent = hasQueueFamilyPresentSupport(physicalDevice, index, surface);

		if (kSupportsGraphics && !selection.hasGraphicsFamily) {
			selection.graphicsFamily = index;
			selection.hasGraphicsFamily = true;
		}

		if (kSupportsCompute && !selection.hasComputeFamily) {
			selection.computeFamily = index;
			selection.hasComputeFamily = true;
		}

		if (kSupportsTransfer && !selection.hasTransferFamily) {
			selection.transferFamily = index;
			selection.hasTransferFamily = true;
		}

		if (kSupportsPresent && !selection.hasPresentFamily) {
			selection.presentFamily = index;
			selection.hasPresentFamily = true;
		}
	}

	return selection;
}

void initialiseDeviceFeatures(DeviceFeatures* features) {
	TRIVIAL_ASSERT(features != nullptr);

	*features = {};

	features->features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	features->features.pNext = &features->vulkan13;

	features->vulkan13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
}

DeviceFeatures makeRequiredDeviceFeatures() {
	DeviceFeatures features = {};
	initialiseDeviceFeatures(&features);

	features.vulkan13.synchronization2 = VK_TRUE;
	features.vulkan13.dynamicRendering = VK_TRUE;

	return features;
}

DeviceFeatures queryDeviceFeatures(VkPhysicalDevice physicalDevice) {
	TRIVIAL_ASSERT(physicalDevice != VK_NULL_HANDLE);

	DeviceFeatures features = {};
	initialiseDeviceFeatures(&features);

	vkGetPhysicalDeviceFeatures2(physicalDevice, &features.features);

	return features;
}

bool supportsRequiredVulkan13Features(const VkPhysicalDeviceVulkan13Features* supportedFeatures,
                                      const VkPhysicalDeviceVulkan13Features* requiredFeatures) {
	TRIVIAL_ASSERT(supportedFeatures != nullptr);
	TRIVIAL_ASSERT(requiredFeatures != nullptr);

	// TODO: Expand for all options
	if (requiredFeatures->synchronization2 == VK_TRUE && supportedFeatures->synchronization2 != VK_TRUE) {
		return false;
	}
	if (requiredFeatures->dynamicRendering == VK_TRUE && supportedFeatures->dynamicRendering != VK_TRUE) {
		return false;
	}

	return true;
}

bool supportsRequiredDeviceFeatures(const DeviceFeatures* supportedFeatures, const DeviceFeatures* requiredFeatures) {
	TRIVIAL_ASSERT(supportedFeatures != nullptr);
	TRIVIAL_ASSERT(requiredFeatures != nullptr);

	return supportsRequiredVulkan13Features(&supportedFeatures->vulkan13, &requiredFeatures->vulkan13);
}

// TODO: Later add a proper selection for the best physicalDevice available and maybe allow for user switching and setting this
PhysicalDeviceSelection selectPhysicalDevice(std::span<const VkPhysicalDevice> physicalDevices, VkSurfaceKHR surface) {
	TRIVIAL_ASSERT(surface != VK_NULL_HANDLE);

	const DeviceFeatures kRequiredFeatures = makeRequiredDeviceFeatures();

	PhysicalDeviceSelection selection = {};

	for (VkPhysicalDevice physicalDevice : physicalDevices) {
		const DeviceFeatures kSupportedFeatures = queryDeviceFeatures(physicalDevice);

		if (!supportsRequiredDeviceFeatures(&kSupportedFeatures, &kRequiredFeatures)) {
			continue;
		}

		const std::vector<VkExtensionProperties> kAvailableExtensions = enumerateDeviceExtensions(physicalDevice);

		if (!supportsRequiredDeviceExtensions(kAvailableExtensions)) {
			continue;
		}

		const std::vector<VkQueueFamilyProperties> kQueueFamilies = enumerateQueueFamilies(physicalDevice);

		const QueueFamilySelection kQueueSelection = selectQueueFamilies(physicalDevice, kQueueFamilies, surface);

		if (!kQueueSelection.hasGraphicsFamily) {
			continue;
		}

		if (!kQueueSelection.hasPresentFamily) {
			continue;
		}

		selection.physicalDevice = physicalDevice;
		selection.queueFamilies = kQueueSelection;
		break;
	}

	TRIVIAL_ASSERT(selection.physicalDevice != VK_NULL_HANDLE);
	TRIVIAL_ASSERT(selection.queueFamilies.hasGraphicsFamily);
	TRIVIAL_ASSERT(selection.queueFamilies.hasPresentFamily);

	return selection;
}

VkDeviceQueueCreateInfo makeDeviceQueueCreateInfo(std::uint32_t queueFamily) {
	static constexpr float s_kQueuePriority = 1.0F;

	VkDeviceQueueCreateInfo createInfo = {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
	                                      .queueFamilyIndex = queueFamily,
	                                      .queueCount = 1, // TODO: Will want to change this later
	                                      .pQueuePriorities = &s_kQueuePriority};

	return createInfo;
}

VkDeviceCreateInfo makeDeviceCreateInfo(const VkDeviceQueueCreateInfo* queueCreateInfo,
                                        const DeviceFeatures* requiredFeatures,
                                        const DeviceSelection* selection) {
	TRIVIAL_ASSERT(queueCreateInfo != nullptr);
	TRIVIAL_ASSERT(requiredFeatures != nullptr);
	TRIVIAL_ASSERT(selection != nullptr);

	VkDeviceCreateInfo createInfo = {.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
	                                 .pNext = &requiredFeatures->vulkan13,
	                                 .queueCreateInfoCount = 1,
	                                 .pQueueCreateInfos = queueCreateInfo,
	                                 .enabledExtensionCount = static_cast<std::uint32_t>(selection->extensions.size()),
	                                 .ppEnabledExtensionNames = selection->extensions.data()};

	return createInfo;
}

// TODO: change later to support just compute gpu
VkDevice createDevice(VkPhysicalDevice physicalDevice, const QueueFamilySelection* queueFamilies) {
	TRIVIAL_ASSERT(physicalDevice != VK_NULL_HANDLE);
	TRIVIAL_ASSERT(queueFamilies != nullptr);
	TRIVIAL_ASSERT(queueFamilies->hasGraphicsFamily);
	TRIVIAL_ASSERT(queueFamilies->hasPresentFamily);
	TRIVIAL_ASSERT(queueFamilies->graphicsFamily == queueFamilies->presentFamily);

	const DeviceFeatures kRequiredFeatures = makeRequiredDeviceFeatures();
	const DeviceFeatures kSupportedFeatures = queryDeviceFeatures(physicalDevice);

	TRIVIAL_ASSERT(supportsRequiredDeviceFeatures(&kSupportedFeatures, &kRequiredFeatures));

	const VkDeviceQueueCreateInfo kQueueCreateInfo = makeDeviceQueueCreateInfo(queueFamilies->graphicsFamily);

	const std::vector<VkExtensionProperties> kAvailableExtensions = enumerateDeviceExtensions(physicalDevice);
	const DeviceSelection kSelection = makeDeviceSelection(kAvailableExtensions);

	const VkDeviceCreateInfo kCreateInfo = makeDeviceCreateInfo(&kQueueCreateInfo, &kRequiredFeatures, &kSelection);

	VkDevice device = VK_NULL_HANDLE;

	const VkResult kResult = vkCreateDevice(physicalDevice, &kCreateInfo, nullptr, &device);

	if (kResult != VK_SUCCESS) {
		TRIVIAL_LOG_ERROR("vkCreateDevice failed");
		TRIVIAL_LOG_ERROR(resultName(kResult));
	}

	TRIVIAL_ASSERT(kResult == VK_SUCCESS);
	TRIVIAL_ASSERT(device != VK_NULL_HANDLE);

	return device;
}

VkQueue getDeviceQueue(VkDevice device, std::uint32_t queueFamily) {
	TRIVIAL_ASSERT(device != VK_NULL_HANDLE);

	VkQueue queue = VK_NULL_HANDLE;

	vkGetDeviceQueue(device, queueFamily, 0, &queue); // TODO: Migrate from queueIndex of 0

	TRIVIAL_ASSERT(queue != VK_NULL_HANDLE);

	return queue;
}

} // namespace

Backend::Backend(const EngineConfig* config, platform::Window* window)
    : m_instance(createInstance(config))
#if TRIVIAL_ENABLE_VULKAN_VALIDATION
    , m_debugMessenger(createDebugMessenger(m_instance))
#endif // TRIVIAL_ENABLE_VULKAN_VALIDATION
    , m_surface(window->createVulkanSurface(m_instance)) {
	const std::vector<VkPhysicalDevice> kPhysicalDevices = enumeratePhysicalDevices(m_instance);

	const PhysicalDeviceSelection kPhysicalDeviceSelection = selectPhysicalDevice(kPhysicalDevices, m_surface);

	m_physicalDevice = kPhysicalDeviceSelection.physicalDevice;
	m_graphicsFamily = kPhysicalDeviceSelection.queueFamilies.graphicsFamily;
	m_presentFamily = kPhysicalDeviceSelection.queueFamilies.presentFamily;

	m_device = createDevice(m_physicalDevice, &kPhysicalDeviceSelection.queueFamilies);

	m_graphicsQueue = getDeviceQueue(m_device, m_graphicsFamily);
	m_presentQueue = getDeviceQueue(m_device, m_presentFamily);
}

Backend::~Backend() {
	if (m_device != VK_NULL_HANDLE) {
		vkDestroyDevice(m_device, nullptr);
		m_device = VK_NULL_HANDLE;
	}

	m_graphicsQueue = VK_NULL_HANDLE;
	m_graphicsFamily = 0;

	m_presentQueue = VK_NULL_HANDLE;
	m_presentFamily = 0;

	m_physicalDevice = VK_NULL_HANDLE;

	if (m_surface != VK_NULL_HANDLE) {
		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
		m_surface = VK_NULL_HANDLE;
	}

#if TRIVIAL_ENABLE_VULKAN_VALIDATION
	destroyDebugMessenger(m_instance, m_debugMessenger);
	m_debugMessenger = VK_NULL_HANDLE;
#endif // TRIVIAL_ENABLE_VULKAN_VALIDATION

	if (m_instance != VK_NULL_HANDLE) {
		vkDestroyInstance(m_instance, nullptr);
		m_instance = VK_NULL_HANDLE;
	}
}

GraphicsApi Backend::graphicsApi() const {
	return GraphicsApi::Vulkan;
}

void Backend::waitIdle() {
	if (m_device == VK_NULL_HANDLE) {
		return;
	}

	const VkResult kResult = vkDeviceWaitIdle(m_device);

	if (kResult != VK_SUCCESS) {
		TRIVIAL_LOG_ERROR("vkDeviceWaitIdle failed");
		TRIVIAL_LOG_ERROR(resultName(kResult));
	}

	TRIVIAL_ASSERT(kResult == VK_SUCCESS);
}

} // namespace trivial::internal::rhi::vulkan

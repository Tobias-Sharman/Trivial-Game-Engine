#ifndef TRIVIAL_INTERNAL_RHI_VULKAN_BACKEND_H
#define TRIVIAL_INTERNAL_RHI_VULKAN_BACKEND_H

#include <cstdint>

#include <vulkan/vulkan.h>

#include <trivial/engine_config.h>
#include <trivial/internal/platform/window.h>
#include <trivial/internal/rhi/backend.h>

namespace trivial::internal::rhi::vulkan {

class Backend final : public rhi::Backend {
public:
	Backend() = delete;
	explicit Backend(const EngineConfig* config, platform::Window* window);

	~Backend() override;

	Backend(const Backend&) = delete;
	Backend& operator=(const Backend&) = delete;

	Backend(Backend&&) = delete;
	Backend& operator=(Backend&&) = delete;

	[[nodiscard]] GraphicsApi graphicsApi() const override;

	void waitIdle() override;

private:
	VkInstance m_instance = VK_NULL_HANDLE;

#if TRIVIAL_ENABLE_VULKAN_VALIDATION
	VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
#endif // TRIVIAL_ENABLE_VULKAN_VALIDATION

	VkSurfaceKHR m_surface = VK_NULL_HANDLE;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_device = VK_NULL_HANDLE;
	VkQueue m_graphicsQueue = VK_NULL_HANDLE;
	VkQueue m_presentQueue = VK_NULL_HANDLE;
	std::uint32_t m_graphicsFamily = 0;
	std::uint32_t m_presentFamily = 0;
};

} // namespace trivial::internal::rhi::vulkan

#endif // TRIVIAL_INTERNAL_RHI_VULKAN_BACKEND_H

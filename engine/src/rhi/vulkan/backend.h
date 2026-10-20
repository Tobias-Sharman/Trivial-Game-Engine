#ifndef TRIVIAL_SRC_RHI_VULKAN_BACKEND_H
#define TRIVIAL_SRC_RHI_VULKAN_BACKEND_H

#include <cstdint>
#include <span>
#include <vector>

#include <vulkan/vulkan_core.h>

#include <trivial/core/application_info.h>
#include <trivial/core/config.h>
#include <trivial/core/graphics_api.h>
#include <trivial/core/math/affine2.h>
#include <trivial/core/math/vec4.h>
#include <trivial/platform/window.h>
#include <trivial/platform/window_types.h>
#include <trivial/rhi/backend.h>
#include <trivial/rhi/mesh_types.h>

#include "rhi/vulkan/allocator.h"
#include "rhi/vulkan/command.h"
#include "rhi/vulkan/mesh.h"
#include "rhi/vulkan/swapchain.h"
#include "rhi/vulkan/sync.h"

namespace trivial::rhi::vulkan {

class Backend final : public rhi::Backend {
public:
	explicit Backend(const ApplicationInfo& applicationInfo, const platform::Window& window) noexcept;

	~Backend() override;

	Backend(const Backend&) = delete;
	Backend& operator=(const Backend&) = delete;

	Backend(Backend&&) = delete;
	Backend& operator=(Backend&&) = delete;

	[[nodiscard]] bool beginFrame(std::uint64_t frameIndex) noexcept override;
	void endFrame() noexcept override;

	[[nodiscard]] MeshHandle createMesh(std::span<const Vertex2> vertices,
	                                    std::span<const std::uint16_t> indices) noexcept override;
	void drawMesh(MeshHandle handle, const math::Affine2f& transform, const math::Vec4f& tint) noexcept override;
	void destroyMesh(MeshHandle handle) noexcept override;

	[[nodiscard]] GraphicsApi graphicsApi() const noexcept override;

	void waitIdle() noexcept override;

	void resize(platform::WindowSize size) noexcept override;

private:
	void beginRendering() noexcept;
	void endRendering() noexcept;

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
	VmaAllocator m_allocator = VK_NULL_HANDLE;
	std::vector<MeshData> m_meshes;
	VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
	VkPipeline m_pipeline = VK_NULL_HANDLE;
	SwapchainState m_swapchainState = {};
	FrameSyncState m_syncState = {};
	CommandState m_commandState{};

	std::uint32_t m_currentImageSlot = 0;
	std::uint32_t m_currentImageIndex = 0;
};

} // namespace trivial::rhi::vulkan

#endif // TRIVIAL_SRC_RHI_VULKAN_BACKEND_H

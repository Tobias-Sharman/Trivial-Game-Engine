#ifndef TRIVIAL_SRC_RHI_VULKAN_BACKEND_H
#define TRIVIAL_SRC_RHI_VULKAN_BACKEND_H

#include <cstdint>

#include <vulkan/vulkan.h>

#include <trivial/engine_config.h>
#include <trivial/platform/window.h>
#include <trivial/rhi/backend.h>

#include "rhi/vulkan/allocator.h"
#include "rhi/vulkan/command.h"
#include "rhi/vulkan/mesh.h"
#include "rhi/vulkan/swapchain.h"
#include "rhi/vulkan/sync.h"

namespace trivial::rhi::vulkan {

class Backend final : public rhi::Backend {
public:
	explicit Backend(const EngineConfig* config, platform::Window* window) noexcept;

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

	void resize(WindowSize size) noexcept override;

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

// TODO: Physical device selection, best device initially and later allow for user to switch
//       Queue family selection, for stuff like parallel upload alongside graphics/compute (transfer queue etc.)
//       Compute only mode for headless uses of the engine
//       Swapchain image count currently always requests minImageCount + 1 (typically yields triple buffering)
//       expose as an explicit double/triple buffering (or present-mode) choice once there's a settings/config system to
//       surface it through
//       Expose the option for swapchain format preferences rather than hardcoding
//       Expose present style as a configuration rather than hardcoded vsync style it is now
//       Depth attachment
//       Shaders
//       Non-primitive shapes
//       Mesh-handle with genearations -> fold into ecs maybe once that is in place
//       Consider migrating to timeline semaphores if sync complexity grows (multiple queues, async compute, more
//       complex frame dependencies) - see https://www.khronos.org/blog/vulkan-timeline-semaphores
//       Multi-threaded command recording -> would need one pool per (thread x swapchain image) pair
//       Mesh handles on GPU API switch -> need tracking through context or something
//
// For future reference:
//       https://www.howtovulkan.com/

#endif // TRIVIAL_SRC_RHI_VULKAN_BACKEND_H

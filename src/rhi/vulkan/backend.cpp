#include "rhi/vulkan/backend.h"

#include <cstring>
#include <span>
#include <vector>

#include <trivial/core/assert.h>
#include <trivial/core/config.h>
#include <trivial/core/log.h>

#include "rhi/vulkan/device.h"
#include "rhi/vulkan/instance.h"
#include "rhi/vulkan/physical_device.h"
#include "rhi/vulkan/pipeline.h"
#include "rhi/vulkan/result.h"

#if TRIVIAL_ENABLE_VULKAN_VALIDATION

#include "rhi/vulkan/debug_messenger.h"

#endif // TRIVIAL_ENABLE_VULKAN_VALIDATION

namespace {

void transitionImageLayout(VkCommandBuffer commandBuffer,
                           VkImage image,
                           VkImageLayout oldLayout,
                           VkImageLayout newLayout,
                           VkPipelineStageFlags2 srcStage,
                           VkAccessFlags2 srcAccess,
                           VkPipelineStageFlags2 dstStage,
                           VkAccessFlags2 dstAccess) noexcept {
	const VkImageMemoryBarrier2 kBarrier = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
	                                        .pNext = nullptr,
	                                        .srcStageMask = srcStage,
	                                        .srcAccessMask = srcAccess,
	                                        .dstStageMask = dstStage,
	                                        .dstAccessMask = dstAccess,
	                                        .oldLayout = oldLayout,
	                                        .newLayout = newLayout,
	                                        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	                                        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	                                        .image = image,
	                                        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	                                                             .baseMipLevel = 0,
	                                                             .levelCount = 1,
	                                                             .baseArrayLayer = 0,
	                                                             .layerCount = 1}};

	const VkDependencyInfo kDependencyInfo = {.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
	                                          .pNext = nullptr,
	                                          .dependencyFlags = 0,
	                                          .memoryBarrierCount = 0,
	                                          .pMemoryBarriers = nullptr,
	                                          .bufferMemoryBarrierCount = 0,
	                                          .pBufferMemoryBarriers = nullptr,
	                                          .imageMemoryBarrierCount = 1,
	                                          .pImageMemoryBarriers = &kBarrier};

	vkCmdPipelineBarrier2(commandBuffer, &kDependencyInfo);
}

} // namespace

namespace trivial::rhi::vulkan {

Backend::Backend(const EngineConfig* config, platform::Window* window) noexcept
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

	m_allocator = createAllocator(m_instance, m_physicalDevice, m_device);

	m_swapchainState = createSwapchain({.physicalDevice = m_physicalDevice,
	                                    .device = m_device,
	                                    .surface = m_surface,
	                                    .requestedSize = window->framebufferSize(),
	                                    .graphicsFamily = m_graphicsFamily,
	                                    .presentFamily = m_presentFamily,
	                                    .oldSwapchain = VK_NULL_HANDLE});

	const auto kImageCount = static_cast<std::uint32_t>(m_swapchainState.images.size());

	m_syncState = createFrameSyncState(m_device, kImageCount);
	m_commandState = createCommandState(m_device, m_graphicsFamily, kImageCount);

	m_pipelineLayout = createPipelineLayout(m_device);

	VkShaderModule vertexModule
	    = createShaderModule(m_device, std::string(TRIVIAL_SHADER_DIR) + "flat_colour.vert.spv");
	VkShaderModule fragmentModule
	    = createShaderModule(m_device, std::string(TRIVIAL_SHADER_DIR) + "flat_colour.frag.spv");

	m_pipeline = createGraphicsPipeline(m_device,
	                                    m_pipelineLayout,
	                                    m_swapchainState.imageFormat,
	                                    vertexModule,
	                                    fragmentModule);

	destroyShaderModule(m_device, vertexModule);
	destroyShaderModule(m_device, fragmentModule);
}
Backend::~Backend() {
	waitIdle();

	destroyGraphicsPipeline(m_device, m_pipeline);
	destroyPipelineLayout(m_device, m_pipelineLayout);

	for (MeshData& mesh : m_meshes) {
		destroyMeshData(m_allocator, &mesh);
	}
	m_meshes.clear();

	destroyCommandState(m_device, &m_commandState);
	destroyFrameSyncState(m_device, &m_syncState);
	destroySwapchain(m_device, &m_swapchainState);

	destroyAllocator(m_allocator);

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

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
bool Backend::beginFrame(std::uint64_t frameIndex) noexcept {
	if (m_swapchainState.imageExtent.width == 0 || m_swapchainState.imageExtent.height == 0) {
		return false;
	}

	const auto kImageCount = static_cast<std::uint32_t>(m_swapchainState.images.size());
	m_currentImageSlot = static_cast<std::uint32_t>(frameIndex % kImageCount);

	VkFence_T* const kFence = m_syncState.inFlightFences[m_currentImageSlot];

	VkResult result = vkWaitForFences(m_device, 1, &kFence, VK_TRUE, UINT64_MAX);

	TRIVIAL_VK_CHECK("vkWaitForFences failed", result);

	std::uint32_t imageIndex = 0;

	result = vkAcquireNextImageKHR(m_device,
	                               m_swapchainState.swapchain,
	                               UINT64_MAX,
	                               m_syncState.imageAvailableSemaphores[m_currentImageSlot],
	                               VK_NULL_HANDLE,
	                               &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		resize({.height = m_swapchainState.imageExtent.width, .width = m_swapchainState.imageExtent.height});
		return false;
	}

	TRIVIAL_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR); // NOLINT(readability-simplify-boolean-expr)

	result = vkResetFences(m_device, 1, &kFence);

	TRIVIAL_VK_CHECK("vkResetFences failed", result);

	m_currentImageIndex = imageIndex;

	VkCommandBuffer_T* const kCommandBuffer = m_commandState.commandBuffers[m_currentImageSlot];

	result = vkResetCommandBuffer(kCommandBuffer, 0);

	TRIVIAL_VK_CHECK("vkResetCommandBuffer failed", result);

	static constexpr VkCommandBufferBeginInfo s_kBeginInfo = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	                                                          .pNext = nullptr,
	                                                          .flags = 0,
	                                                          .pInheritanceInfo = nullptr};

	result = vkBeginCommandBuffer(kCommandBuffer, &s_kBeginInfo);

	TRIVIAL_VK_CHECK("vkBeginCommandBuffer failed", result);

	beginRendering();

	return true;
}

void Backend::endFrame() noexcept {
	endRendering();

	VkCommandBuffer commandBuffer = m_commandState.commandBuffers[m_currentImageSlot];

	VkResult result = vkEndCommandBuffer(commandBuffer);

	TRIVIAL_VK_CHECK("vkEndCommandBuffer failed", result);

	VkSemaphore_T* const kWaitSemaphore = m_syncState.imageAvailableSemaphores[m_currentImageSlot];
	VkSemaphore_T* const kSignalSemaphore = m_syncState.renderFinishedSemaphores[m_currentImageIndex];

	const VkSemaphoreSubmitInfo kWaitSemaphoreInfo = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
	                                                  .pNext = nullptr,
	                                                  .semaphore = kWaitSemaphore,
	                                                  .value = 0,
	                                                  .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
	                                                  .deviceIndex = 0};

	const VkCommandBufferSubmitInfo kCommandBufferSubmitInfo = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
	                                                            .pNext = nullptr,
	                                                            .commandBuffer = commandBuffer,
	                                                            .deviceMask = 0};

	const VkSemaphoreSubmitInfo kSignalSemaphoreInfo = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
	                                                    .pNext = nullptr,
	                                                    .semaphore = kSignalSemaphore,
	                                                    .value = 0,
	                                                    .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
	                                                    .deviceIndex = 0};

	const VkSubmitInfo2 kSubmitInfo = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
	                                   .pNext = nullptr,
	                                   .flags = 0,
	                                   .waitSemaphoreInfoCount = 1,
	                                   .pWaitSemaphoreInfos = &kWaitSemaphoreInfo,
	                                   .commandBufferInfoCount = 1,
	                                   .pCommandBufferInfos = &kCommandBufferSubmitInfo,
	                                   .signalSemaphoreInfoCount = 1,
	                                   .pSignalSemaphoreInfos = &kSignalSemaphoreInfo};

	VkFence fence = m_syncState.inFlightFences[m_currentImageSlot];

	result = vkQueueSubmit2(m_graphicsQueue, 1, &kSubmitInfo, fence);

	TRIVIAL_VK_CHECK("vkQueueSubmit2 failed", result);

	VkSwapchainKHR_T* const kSwapchain = m_swapchainState.swapchain;

	const VkPresentInfoKHR kPresentInfo = {.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
	                                       .pNext = nullptr,
	                                       .waitSemaphoreCount = 1,
	                                       .pWaitSemaphores = &kSignalSemaphore,
	                                       .swapchainCount = 1,
	                                       .pSwapchains = &kSwapchain,
	                                       .pImageIndices = &m_currentImageIndex,
	                                       .pResults = nullptr};

	result = vkQueuePresentKHR(m_presentQueue, &kPresentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		resize({.height = m_swapchainState.imageExtent.width, .width = m_swapchainState.imageExtent.height});
		return;
	}

	TRIVIAL_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR); // NOLINT(readability-simplify-boolean-expr)
}

MeshHandle Backend::createMesh(std::span<const Vertex2> vertices, std::span<const std::uint16_t> indices) noexcept {
	const MeshData kMesh = createMeshData(m_allocator, vertices, indices);

	m_meshes.push_back(kMesh);

	return static_cast<MeshHandle>(m_meshes.size() - 1);
}

void Backend::drawMesh(MeshHandle handle, const math::Affine2f& transform, const math::Vec4f& tint) noexcept {
	TRIVIAL_ASSERT(handle < m_meshes.size());

	const MeshData& mesh = m_meshes[handle];

	VkCommandBuffer commandBuffer = m_commandState.commandBuffers[m_currentImageSlot];
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

	VkBuffer vertexBuffer = mesh.buffer;
	static constexpr VkDeviceSize s_kVertexOffset = 0;
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &s_kVertexOffset);
	vkCmdBindIndexBuffer(commandBuffer, mesh.buffer, mesh.indexOffset, VK_INDEX_TYPE_UINT16);

	const PushConstants kPushConstants = {.tint = tint, .transform = transform};
	vkCmdPushConstants(commandBuffer,
	                   m_pipelineLayout,
	                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
	                   0,
	                   sizeof(PushConstants),
	                   &kPushConstants);

	vkCmdDrawIndexed(commandBuffer, mesh.indexCount, 1, 0, 0, 0);
}

void Backend::destroyMesh(MeshHandle handle) noexcept {
	TRIVIAL_ASSERT(handle < m_meshes.size());

	destroyMeshData(m_allocator, &m_meshes[handle]);
}

GraphicsApi Backend::graphicsApi() const noexcept {
	return GraphicsApi::Vulkan;
}

void Backend::waitIdle() noexcept {
	if (m_device == VK_NULL_HANDLE) {
		return;
	}

	const VkResult kResult = vkDeviceWaitIdle(m_device);

	TRIVIAL_VK_CHECK("vkDeviceWaitIdle failed", kResult);
}

void Backend::resize(WindowSize size) noexcept {
	if (size.width == 0 || size.height == 0) {
		return;
	}

	waitIdle();

	destroyCommandState(m_device, &m_commandState);
	destroyFrameSyncState(m_device, &m_syncState);

	SwapchainState kOldSwapchainState = m_swapchainState;

	m_swapchainState = createSwapchain({.physicalDevice = m_physicalDevice,
	                                    .device = m_device,
	                                    .surface = m_surface,
	                                    .requestedSize = size,
	                                    .graphicsFamily = m_graphicsFamily,
	                                    .presentFamily = m_presentFamily,
	                                    .oldSwapchain = kOldSwapchainState.swapchain});

	destroySwapchain(m_device, &kOldSwapchainState);

	const std::uint32_t kImageCount = static_cast<std::uint32_t>(m_swapchainState.images.size());

	m_syncState = createFrameSyncState(m_device, kImageCount);
	m_commandState = createCommandState(m_device, m_graphicsFamily, kImageCount);
}

void Backend::beginRendering() noexcept {
	VkCommandBuffer commandBuffer = m_commandState.commandBuffers[m_currentImageSlot];
	VkImage image = m_swapchainState.images[m_currentImageIndex];

	transitionImageLayout(commandBuffer,
	                      image,
	                      VK_IMAGE_LAYOUT_UNDEFINED,
	                      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	                      VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
	                      VK_ACCESS_2_NONE,
	                      VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
	                      VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);

	static constexpr VkClearValue s_kClearColor = {.color = {.float32 = {0.0F, 0.0F, 0.0F, 1.0F}}};

	const VkRenderingAttachmentInfo kColorAttachment = {.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
	                                                    .pNext = nullptr,
	                                                    .imageView = m_swapchainState.imageViews[m_currentImageIndex],
	                                                    .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	                                                    .resolveMode = VK_RESOLVE_MODE_NONE,
	                                                    .resolveImageView = VK_NULL_HANDLE,
	                                                    .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	                                                    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
	                                                    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
	                                                    .clearValue = s_kClearColor};

	const VkRenderingInfo kRenderingInfo
	    = {.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
	       .pNext = nullptr,
	       .flags = 0,
	       .renderArea = {.offset = {.x = 0, .y = 0}, .extent = m_swapchainState.imageExtent},
	       .layerCount = 1,
	       .viewMask = 0,
	       .colorAttachmentCount = 1,
	       .pColorAttachments = &kColorAttachment,
	       .pDepthAttachment = nullptr,
	       .pStencilAttachment = nullptr};

	vkCmdBeginRendering(commandBuffer, &kRenderingInfo);

	const VkViewport kViewport = {.x = 0.0F,
	                              .y = 0.0F,
	                              .width = static_cast<float>(m_swapchainState.imageExtent.width),
	                              .height = static_cast<float>(m_swapchainState.imageExtent.height),
	                              .minDepth = 0.0F,
	                              .maxDepth = 1.0F};
	vkCmdSetViewport(commandBuffer, 0, 1, &kViewport);

	const VkRect2D kScissor = {.offset = {.x = 0, .y = 0}, .extent = m_swapchainState.imageExtent};
	vkCmdSetScissor(commandBuffer, 0, 1, &kScissor);
}

void Backend::endRendering() noexcept {
	VkCommandBuffer commandBuffer = m_commandState.commandBuffers[m_currentImageSlot];
	VkImage image = m_swapchainState.images[m_currentImageIndex];

	vkCmdEndRendering(commandBuffer);

	transitionImageLayout(commandBuffer,
	                      image,
	                      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	                      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	                      VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
	                      VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
	                      VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
	                      VK_ACCESS_2_NONE);
}

} // namespace trivial::rhi::vulkan

#ifndef TRIVIAL_SRC_RHI_VULKAN_PIPELINE_H
#define TRIVIAL_SRC_RHI_VULKAN_PIPELINE_H

#include <string>

#include <vulkan/vulkan.h>

#include <trivial/core/math/affine2.h>
#include <trivial/core/math/vec4.h>

namespace trivial::rhi::vulkan {

struct PushConstants {
	math::Vec4f tint;
	math::Affine2f transform;
};

VkShaderModule createShaderModule(VkDevice device, const std::string& spirvPath) noexcept;
void destroyShaderModule(VkDevice device, VkShaderModule module) noexcept;

VkPipelineLayout createPipelineLayout(VkDevice device) noexcept;
void destroyPipelineLayout(VkDevice device, VkPipelineLayout layout) noexcept;

VkPipeline createGraphicsPipeline(VkDevice device,
                                  VkPipelineLayout layout,
                                  VkFormat colorFormat,
                                  VkShaderModule vertexModule,
                                  VkShaderModule fragmentModule) noexcept;
void destroyGraphicsPipeline(VkDevice device, VkPipeline pipeline) noexcept;

static_assert(sizeof(PushConstants) == 40);
static_assert(std::is_trivially_copyable_v<PushConstants>);
static_assert(std::is_standard_layout_v<PushConstants>);

} // namespace trivial::rhi::vulkan

#endif // TRIVIAL_SRC_RHI_VULKAN_PIPELINE_H

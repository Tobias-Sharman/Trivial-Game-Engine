#include "rhi/vulkan/pipeline.h"

#include <fstream>
#include <vector>

#include <trivial/core/assert.h>

#include "rhi/vulkan/result.h"
#include "trivial/rhi/mesh_types.h"

namespace {

VkVertexInputBindingDescription makeVertexBinding() noexcept {
	return {.binding = 0, .stride = sizeof(trivial::rhi::Vertex2), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};
}

std::array<VkVertexInputAttributeDescription, 2> makeVertexAttributes() noexcept {
	return {{{.location = 0,
	          .binding = 0,
	          .format = VK_FORMAT_R32G32_SFLOAT,
	          .offset = offsetof(trivial::rhi::Vertex2, position)},
	         {.location = 1,
	          .binding = 0,
	          .format = VK_FORMAT_R8G8B8A8_UNORM,
	          .offset = offsetof(trivial::rhi::Vertex2, colour)}}};
}

VkPipelineInputAssemblyStateCreateInfo makeInputAssemblyState() noexcept {
	return {.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
	        .pNext = nullptr,
	        .flags = 0,
	        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	        .primitiveRestartEnable = VK_FALSE};
}

std::array<VkPipelineShaderStageCreateInfo, 2> makeShaderStages(VkShaderModule vertexModule,
                                                                VkShaderModule fragmentModule) noexcept {
	return {{{.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	          .pNext = nullptr,
	          .flags = 0,
	          .stage = VK_SHADER_STAGE_VERTEX_BIT,
	          .module = vertexModule,
	          .pName = "main",
	          .pSpecializationInfo = nullptr},
	         {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	          .pNext = nullptr,
	          .flags = 0,
	          .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
	          .module = fragmentModule,
	          .pName = "main",
	          .pSpecializationInfo = nullptr}}};
}

VkPipelineViewportStateCreateInfo makeViewportState() noexcept {
	return {.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
	        .pNext = nullptr,
	        .flags = 0,
	        .viewportCount = 1,
	        .pViewports = nullptr,
	        .scissorCount = 1,
	        .pScissors = nullptr};
}

std::array<VkDynamicState, 2> makeDynamicStates() noexcept {
	return {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
}

VkPipelineDynamicStateCreateInfo makeDynamicState(const std::array<VkDynamicState, 2>& states) noexcept {
	return {.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
	        .pNext = nullptr,
	        .flags = 0,
	        .dynamicStateCount = static_cast<std::uint32_t>(states.size()),
	        .pDynamicStates = states.data()};
}

VkPipelineRenderingCreateInfo makeRenderingCreateInfo(const VkFormat& colorFormat) noexcept {
	return {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
	        .pNext = nullptr,
	        .viewMask = 0,
	        .colorAttachmentCount = 1,
	        .pColorAttachmentFormats = &colorFormat,
	        .depthAttachmentFormat = VK_FORMAT_UNDEFINED,
	        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED};
}

VkPipelineColorBlendAttachmentState makeColorBlendAttachment() noexcept {
	return {.blendEnable = VK_FALSE,
	        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
	        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
	        .colorBlendOp = VK_BLEND_OP_ADD,
	        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
	        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
	        .alphaBlendOp = VK_BLEND_OP_ADD,
	        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
	                          | VK_COLOR_COMPONENT_A_BIT};
}

VkPipelineColorBlendStateCreateInfo makeColorBlendState(
    const VkPipelineColorBlendAttachmentState& attachment) noexcept {
	return {.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
	        .pNext = nullptr,
	        .flags = 0,
	        .logicOpEnable = VK_FALSE,
	        .logicOp = VK_LOGIC_OP_COPY,
	        .attachmentCount = 1,
	        .pAttachments = &attachment,
	        .blendConstants = {0.0F, 0.0F, 0.0F, 0.0F}};
}

VkPipelineRasterizationStateCreateInfo makeRasterizationState() noexcept {
	return {.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
	        .pNext = nullptr,
	        .flags = 0,
	        .depthClampEnable = VK_FALSE,
	        .rasterizerDiscardEnable = VK_FALSE,
	        .polygonMode = VK_POLYGON_MODE_FILL,
	        .cullMode = VK_CULL_MODE_NONE,
	        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
	        .depthBiasEnable = VK_FALSE,
	        .depthBiasConstantFactor = 0.0F,
	        .depthBiasClamp = 0.0F,
	        .depthBiasSlopeFactor = 0.0F,
	        .lineWidth = 1.0F};
}

VkPipelineMultisampleStateCreateInfo makeMultisampleState() noexcept {
	return {.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
	        .pNext = nullptr,
	        .flags = 0,
	        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
	        .sampleShadingEnable = VK_FALSE,
	        .minSampleShading = 1.0F,
	        .pSampleMask = nullptr,
	        .alphaToCoverageEnable = VK_FALSE,
	        .alphaToOneEnable = VK_FALSE};
}

} // namespace

namespace trivial::rhi::vulkan {

VkShaderModule createShaderModule(VkDevice device, const std::string& spirvPath) noexcept {
	TRIVIAL_ASSERT(device != VK_NULL_HANDLE);

	std::ifstream file(spirvPath, std::ios::ate | std::ios::binary);

	TRIVIAL_ASSERT(file.is_open());

	const std::streamsize kFileSize = file.tellg();

	TRIVIAL_ASSERT(kFileSize > 0);
	TRIVIAL_ASSERT(kFileSize % 4 == 0); // SPIR-V is a stream of 32-bit words

	std::vector<char> buffer(static_cast<std::size_t>(kFileSize));

	file.seekg(0);
	file.read(buffer.data(), kFileSize);

	const VkShaderModuleCreateInfo kCreateInfo = {.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
	                                              .pNext = nullptr,
	                                              .flags = 0,
	                                              .codeSize = buffer.size(),
	                                              // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	                                              .pCode = reinterpret_cast<const std::uint32_t*>(buffer.data())};

	VkShaderModule module = VK_NULL_HANDLE;

	const VkResult kResult = vkCreateShaderModule(device, &kCreateInfo, nullptr, &module);

	TRIVIAL_VK_CHECK("vkCreateShaderModule failed", kResult);
	TRIVIAL_ASSERT(module != VK_NULL_HANDLE);

	return module;
}

void destroyShaderModule(VkDevice device, VkShaderModule module) noexcept {
	TRIVIAL_ASSERT(device != VK_NULL_HANDLE);

	if (module != VK_NULL_HANDLE) {
		vkDestroyShaderModule(device, module, nullptr);
	}
}

VkPipelineLayout createPipelineLayout(VkDevice device) noexcept {
	TRIVIAL_ASSERT(device != VK_NULL_HANDLE);

	static constexpr VkPushConstantRange s_kPushConstantRange
	    = {.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
	       .offset = 0,
	       .size = sizeof(PushConstants)};

	const VkPipelineLayoutCreateInfo kCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
	                                                .pNext = nullptr,
	                                                .flags = 0,
	                                                .setLayoutCount = 0,
	                                                .pSetLayouts = nullptr,
	                                                .pushConstantRangeCount = 1,
	                                                .pPushConstantRanges = &s_kPushConstantRange};

	VkPipelineLayout layout = VK_NULL_HANDLE;

	const VkResult kResult = vkCreatePipelineLayout(device, &kCreateInfo, nullptr, &layout);

	TRIVIAL_VK_CHECK("vkCreatePipelineLayout failed", kResult);
	TRIVIAL_ASSERT(layout != VK_NULL_HANDLE);

	return layout;
}

void destroyPipelineLayout(VkDevice device, VkPipelineLayout layout) noexcept {
	TRIVIAL_ASSERT(device != VK_NULL_HANDLE);

	if (layout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(device, layout, nullptr);
	}
}

VkPipeline createGraphicsPipeline(VkDevice device,
                                  VkPipelineLayout layout,
                                  VkFormat colorFormat,
                                  VkShaderModule vertexModule,
                                  VkShaderModule fragmentModule) noexcept {
	TRIVIAL_ASSERT(device != VK_NULL_HANDLE);
	TRIVIAL_ASSERT(layout != VK_NULL_HANDLE);
	TRIVIAL_ASSERT(vertexModule != VK_NULL_HANDLE);
	TRIVIAL_ASSERT(fragmentModule != VK_NULL_HANDLE);

	const VkVertexInputBindingDescription kVertexBinding = makeVertexBinding();
	const std::array<VkVertexInputAttributeDescription, 2> kVertexAttributes = makeVertexAttributes();

	const VkPipelineVertexInputStateCreateInfo kVertexInputState
	    = {.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
	       .pNext = nullptr,
	       .flags = 0,
	       .vertexBindingDescriptionCount = 1,
	       .pVertexBindingDescriptions = &kVertexBinding,
	       .vertexAttributeDescriptionCount = static_cast<std::uint32_t>(kVertexAttributes.size()),
	       .pVertexAttributeDescriptions = kVertexAttributes.data()};

	const VkPipelineInputAssemblyStateCreateInfo kInputAssemblyState = makeInputAssemblyState();

	const std::array<VkPipelineShaderStageCreateInfo, 2> kShaderStages = makeShaderStages(vertexModule, fragmentModule);

	const VkPipelineViewportStateCreateInfo kViewportState = makeViewportState();

	const std::array<VkDynamicState, 2> kDynamicStates = makeDynamicStates();
	const VkPipelineDynamicStateCreateInfo kDynamicState = makeDynamicState(kDynamicStates);

	const VkPipelineRenderingCreateInfo kRenderingCreateInfo = makeRenderingCreateInfo(colorFormat);

	const VkPipelineColorBlendAttachmentState kColorBlendAttachment = makeColorBlendAttachment();
	const VkPipelineColorBlendStateCreateInfo kColorBlendState = makeColorBlendState(kColorBlendAttachment);

	const VkPipelineRasterizationStateCreateInfo kRasterizationState = makeRasterizationState();
	const VkPipelineMultisampleStateCreateInfo kMultisampleState = makeMultisampleState();

	const VkGraphicsPipelineCreateInfo kPipelineCreateInfo
	    = {.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
	       .pNext = &kRenderingCreateInfo,
	       .flags = 0,
	       .stageCount = static_cast<std::uint32_t>(kShaderStages.size()),
	       .pStages = kShaderStages.data(),
	       .pVertexInputState = &kVertexInputState,
	       .pInputAssemblyState = &kInputAssemblyState,
	       .pTessellationState = nullptr,
	       .pViewportState = &kViewportState,
	       .pRasterizationState = &kRasterizationState,
	       .pMultisampleState = &kMultisampleState,
	       .pDepthStencilState = nullptr,
	       .pColorBlendState = &kColorBlendState,
	       .pDynamicState = &kDynamicState,
	       .layout = layout,
	       .renderPass = VK_NULL_HANDLE,
	       .subpass = 0,
	       .basePipelineHandle = VK_NULL_HANDLE,
	       .basePipelineIndex = -1};

	VkPipeline pipeline = VK_NULL_HANDLE;

	const VkResult kResult
	    = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &kPipelineCreateInfo, nullptr, &pipeline);

	TRIVIAL_VK_CHECK("vkCreateGraphicsPipelines failed", kResult);
	TRIVIAL_ASSERT(pipeline != VK_NULL_HANDLE);

	return pipeline;
}

void destroyGraphicsPipeline(VkDevice device, VkPipeline pipeline) noexcept {
	TRIVIAL_ASSERT(device != VK_NULL_HANDLE);

	if (pipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(device, pipeline, nullptr);
	}
}

} // namespace trivial::rhi::vulkan

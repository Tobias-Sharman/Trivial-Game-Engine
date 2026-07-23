#ifndef TRIVIAL_SRC_RHI_VULKAN_INSTANCE_H
#define TRIVIAL_SRC_RHI_VULKAN_INSTANCE_H

#include <vulkan/vulkan.h>

#include <trivial/engine_config.h>

namespace trivial::rhi::vulkan {

VkInstance createInstance(const EngineConfig* config);

}

#endif // TRIVIAL_SRC_RHI_VULKAN_INSTANCE_H

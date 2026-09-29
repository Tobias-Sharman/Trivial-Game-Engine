#ifndef TRIVIAL_SRC_RHI_VULKAN_INSTANCE_H
#define TRIVIAL_SRC_RHI_VULKAN_INSTANCE_H

#include <vulkan/vulkan_core.h>

#include <trivial/core/application_info.h>

namespace trivial::rhi::vulkan {

VkInstance createInstance(const ApplicationInfo& applicationInfo) noexcept;

}

#endif // TRIVIAL_SRC_RHI_VULKAN_INSTANCE_H

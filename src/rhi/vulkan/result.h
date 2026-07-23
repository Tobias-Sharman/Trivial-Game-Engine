#ifndef TRIVIAL_SRC_RHI_VULKAN_RESULT_H
#define TRIVIAL_SRC_RHI_VULKAN_RESULT_H

#include <vulkan/vulkan.h>

namespace trivial::rhi::vulkan {

[[nodiscard]] const char* resultName(VkResult result);

} // namespace trivial::rhi::vulkan

#endif // TRIVIAL_SRC_RHI_VULKAN_RESULT_H

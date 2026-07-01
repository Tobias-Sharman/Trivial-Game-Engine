#ifndef TRIVIAL_INTERNAL_RHI_VULKAN_RESULT_H
#define TRIVIAL_INTERNAL_RHI_VULKAN_RESULT_H

#include <vulkan/vulkan.h>

namespace trivial::internal::rhi::vulkan {

[[nodiscard]] const char* resultName(VkResult result);

} // namespace trivial::internal::rhi::vulkan

#endif // TRIVIAL_INTERNAL_RHI_VULKAN_RESULT_H

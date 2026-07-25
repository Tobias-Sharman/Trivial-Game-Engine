#ifndef TRIVIAL_SRC_RHI_VULKAN_MESH_H
#define TRIVIAL_SRC_RHI_VULKAN_MESH_H

#include <cstdint>
#include <span>
#include <vk_mem_alloc.h>

#include <vulkan/vulkan.h>

#include <trivial/rhi/mesh_types.h>

namespace trivial::rhi::vulkan {

struct MeshData {
	VkBuffer buffer = VK_NULL_HANDLE;
	VmaAllocation allocation = VK_NULL_HANDLE;
	VkDeviceSize indexOffset = 0;
	std::uint32_t indexCount = 0;
	VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
};

MeshData createMeshData(VmaAllocator allocator,
                        std::span<const Vertex2> vertices,
                        std::span<const std::uint16_t> indices) noexcept;
void destroyMeshData(VmaAllocator allocator, MeshData* mesh) noexcept;

} // namespace trivial::rhi::vulkan

#endif // TRIVIAL_SRC_RHI_VULKAN_MESH_H

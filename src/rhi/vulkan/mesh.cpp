#include "rhi/vulkan/mesh.h"

#include <cstring>

#include <trivial/core/assert.h>

#include "rhi/vulkan/result.h"

namespace trivial::rhi::vulkan {

MeshData createMeshData(VmaAllocator allocator,
                        std::span<const Vertex2> vertices,
                        std::span<const std::uint16_t> indices) noexcept {
	TRIVIAL_ASSERT(allocator != VK_NULL_HANDLE);
	TRIVIAL_ASSERT(!vertices.empty());
	TRIVIAL_ASSERT(!indices.empty());

	const VkDeviceSize kVertexBytes = vertices.size_bytes();
	const VkDeviceSize kIndexBytes = indices.size_bytes();
	const VkDeviceSize kTotalBytes = kVertexBytes + kIndexBytes;

	const VkBufferCreateInfo kBufferCreateInfo
	    = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
	       .pNext = nullptr,
	       .flags = 0,
	       .size = kTotalBytes,
	       .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	       .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	       .queueFamilyIndexCount = 0,
	       .pQueueFamilyIndices = nullptr};

	const VmaAllocationCreateInfo kAllocationCreateInfo
	    = {.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
	       .usage = VMA_MEMORY_USAGE_AUTO,
	       .requiredFlags = 0,
	       .preferredFlags = 0,
	       .memoryTypeBits = 0,
	       .pool = VK_NULL_HANDLE,
	       .pUserData = nullptr,
	       .priority = 0.0F};

	MeshData mesh = {};
	mesh.indexOffset = kVertexBytes;
	mesh.indexCount = static_cast<std::uint32_t>(indices.size());

	VmaAllocationInfo allocationInfo = {};

	const VkResult kResult = vmaCreateBuffer(allocator,
	                                         &kBufferCreateInfo,
	                                         &kAllocationCreateInfo,
	                                         &mesh.buffer,
	                                         &mesh.allocation,
	                                         &allocationInfo);

	TRIVIAL_VK_CHECK("vmaCreateBuffer failed", kResult);
	TRIVIAL_ASSERT(mesh.buffer != VK_NULL_HANDLE);
	TRIVIAL_ASSERT(allocationInfo.pMappedData != nullptr);
	// NOTE: need changing to handle large assets later without hard crash -> full handling with
	// VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT

	auto* destination = static_cast<std::uint8_t*>(allocationInfo.pMappedData);
	std::memcpy(destination, vertices.data(), static_cast<std::size_t>(kVertexBytes));
	//NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) -> cba to to do subspan to avoid this
	std::memcpy(destination + kVertexBytes, indices.data(), static_cast<std::size_t>(kIndexBytes));

	return mesh;
}

void destroyMeshData(VmaAllocator allocator, MeshData* mesh) noexcept {
	TRIVIAL_ASSERT(allocator != VK_NULL_HANDLE);
	TRIVIAL_ASSERT(mesh != nullptr);

	if (mesh->buffer != VK_NULL_HANDLE) {
		vmaDestroyBuffer(allocator, mesh->buffer, mesh->allocation);
		mesh->buffer = VK_NULL_HANDLE;
		mesh->allocation = VK_NULL_HANDLE;
	}

	mesh->indexOffset = 0;
	mesh->indexCount = 0;
}

} // namespace trivial::rhi::vulkan

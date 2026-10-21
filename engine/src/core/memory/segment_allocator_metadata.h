#ifndef TRIVIAL_SRC_CORE_MEMORY_SEGMENT_ALLOCATOR_METADATA_H
#define TRIVIAL_SRC_CORE_MEMORY_SEGMENT_ALLOCATOR_METADATA_H

#include <cstddef>

namespace trivial::memory {

[[nodiscard]] void* mapMetadata(std::size_t bytes,
                                std::size_t pageSize,
                                std::size_t& outMappingBytes,
                                int& outOsErrorCode) noexcept;

void unmapMetadata(void* payloadBase, std::size_t mappingBytes, std::size_t pageSize) noexcept;

} // namespace trivial::memory

#endif // TRIVIAL_SRC_CORE_MEMORY_SEGMENT_ALLOCATOR_METADATA_H

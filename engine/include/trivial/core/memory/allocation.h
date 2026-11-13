#ifndef TRIVIAL_CORE_MEMORY_ALLOCATION_H
#define TRIVIAL_CORE_MEMORY_ALLOCATION_H

#include <trivial/core/memory/byte_count.h>

namespace trivial::memory {

[[nodiscard]] void* allocate(ByteCount size) noexcept;
[[nodiscard]] void* allocateAtLeast(ByteCount& size) noexcept;
[[nodiscard]] void* allocateZeroed(ByteCount size) noexcept;
[[nodiscard]] void* allocateAligned(ByteCount size, ByteCount alignment) noexcept;

[[nodiscard]] bool tryExpand(void* pointer, ByteCount newSize) noexcept;

void free(void* pointer) noexcept;

[[nodiscard]] ByteCount usableSize(const void* pointer) noexcept;

} // namespace trivial::memory

#endif // TRIVIAL_CORE_MEMORY_ALLOCATION_H

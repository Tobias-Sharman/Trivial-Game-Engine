#include <trivial/core/memory/allocation.h>

#include <cerrno>
#include <cstdlib>
#include <mimalloc.h>

#include <trivial/core/log.h>
#include <trivial/core/memory/byte_count.h>

namespace trivial::memory {

[[nodiscard]] void* allocate(ByteCount size) noexcept {
	void* pointer = mi_malloc(size.toSize());
	if (pointer == nullptr) [[unlikely]] {
		TRIVIAL_LOG_OOM_FATAL("Memory", "allocate failed", size, errno);
		std::abort();
	}

	return pointer;
}

[[nodiscard]] void* allocateAtLeast(ByteCount& size) noexcept {
	const std::size_t kGoodSize = mi_good_size(size.toSize());
	void* pointer = mi_malloc(kGoodSize);
	if (pointer == nullptr) [[unlikely]] {
		TRIVIAL_LOG_OOM_FATAL("Memory", "allocateAtLeast failed", size, errno);
		std::abort();
	}

	size = bytes(kGoodSize);
	return pointer;
}

[[nodiscard]] void* allocateZeroed(ByteCount size) noexcept {
	void* pointer = mi_zalloc(size.toSize());
	if (pointer == nullptr) [[unlikely]] {
		TRIVIAL_LOG_OOM_FATAL("Memory", "allocateZeroed failed", size, errno);
		std::abort();
	}

	return pointer;
}

[[nodiscard]] void* allocateAligned(ByteCount size, ByteCount alignment) noexcept {
	void* pointer = mi_malloc_aligned(size.toSize(), alignment.toSize());
	if (pointer == nullptr) [[unlikely]] {
		TRIVIAL_LOG_OOM_FATAL("Memory", "allocateAligned failed", size, errno);
		std::abort();
	}

	return pointer;
}

[[nodiscard]] bool tryExpand(void* pointer, ByteCount newSize) noexcept {
	return mi_expand(pointer, newSize.toSize()) != nullptr;
}

void free(void* pointer) noexcept {
	mi_free(pointer);
}

[[nodiscard]] ByteCount usableSize(const void* pointer) noexcept {
	return bytes(mi_usable_size(pointer));
}

} // namespace trivial::memory

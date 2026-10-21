#include "core/memory/segment_allocator_metadata.h"

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <sys/mman.h>

#include <trivial/core/log.h>

namespace trivial::memory {

[[nodiscard]] void* mapMetadata(std::size_t bytes,
                                std::size_t pageSize,
                                std::size_t& outMappingBytes,
                                int& outOsErrorCode) noexcept {
	const std::size_t kPayload = (bytes + pageSize - 1) & ~(pageSize - 1);

	if (kPayload == 0 || kPayload > SIZE_MAX - (2 * pageSize)) {
		outOsErrorCode = 0;
		return nullptr;
	}

	const std::size_t kMappingBytes = kPayload + (2 * pageSize);

	void* raw = mmap(nullptr, kMappingBytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (raw == MAP_FAILED) {
		outOsErrorCode = errno;
		return nullptr;
	}

	void* payloadBase = static_cast<char*>(raw) + pageSize; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

	if (mprotect(payloadBase, kPayload, PROT_READ | PROT_WRITE) != 0) {
		outOsErrorCode = errno;
		(void)munmap(raw, kMappingBytes);
		return nullptr;
	}

	outMappingBytes = kMappingBytes;
	return payloadBase;
}

void unmapMetadata(void* payloadBase, std::size_t mappingBytes, std::size_t pageSize) noexcept {
	if (payloadBase == nullptr) {
		return;
	}

	void* raw = static_cast<char*>(payloadBase) - pageSize; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

	if (munmap(raw, mappingBytes) != 0) {
		TRIVIAL_LOG_ERROR_PREFIX("SegmentAllocator", "metadata release failed (munmap)");
	}
}

} // namespace trivial::memory

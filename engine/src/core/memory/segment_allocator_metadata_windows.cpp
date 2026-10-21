#include "core/memory/segment_allocator_metadata.h"

#include <cstddef>
#include <cstdint>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX

#include <windows.h> // IWYU pragma: keep

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

	void* raw = VirtualAlloc(nullptr, kMappingBytes, MEM_RESERVE, PAGE_NOACCESS);
	if (raw == nullptr) {
		outOsErrorCode = static_cast<int>(GetLastError());
		return nullptr;
	}

	void* payloadBase = static_cast<char*>(raw) + pageSize; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

	if (VirtualAlloc(payloadBase, kPayload, MEM_COMMIT, PAGE_READWRITE) == nullptr) {
		outOsErrorCode = static_cast<int>(GetLastError());
		(void)VirtualFree(raw, 0, MEM_RELEASE);
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

	(void)mappingBytes;

	if (VirtualFree(raw, 0, MEM_RELEASE) == 0) {
		TRIVIAL_LOG_ERROR_PREFIX("SegmentAllocator", "metadata release failed (VirtualFree)");
	}
}

} // namespace trivial::memory

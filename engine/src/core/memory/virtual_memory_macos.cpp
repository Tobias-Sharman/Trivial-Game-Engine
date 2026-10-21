#include "core/memory/virtual_memory.h"

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <sys/mman.h>
#include <unistd.h>

#include <trivial/core/assert.h>
#include <trivial/core/log.h>
#include <trivial/core/memory/memory_config.h>

#if TRIVIAL_ENABLE_ASSERTS
#include <trivial/core/memory/memory.h>
#endif // TRIVIAL_ENABLE_ASSERTS

namespace {

[[nodiscard]] std::size_t alignmentOffset(const void* base, std::size_t alignment) noexcept {
	TRIVIAL_ASSERT(alignment > 0);
	TRIVIAL_ASSERT((alignment & (alignment - 1)) == 0);

	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	std::uintptr_t misalignment = reinterpret_cast<std::uintptr_t>(base) & (alignment - 1);
	return misalignment == 0 ? 0 : alignment - static_cast<std::size_t>(misalignment);
}

} // namespace

namespace trivial::memory {

[[nodiscard]] SystemInfo probeSystemInfo() noexcept {
	SystemInfo info;

	const long kPageSize = sysconf(_SC_PAGESIZE);
	TRIVIAL_ASSERT(kPageSize > 0);

	info.pageSize = static_cast<std::size_t>(kPageSize);
	info.allocationGranularity = info.pageSize;

	return info;
}

[[nodiscard]] void* reserveAligned(std::size_t bytes, std::size_t alignment, int& outOsErrorCode) noexcept {
	TRIVIAL_ASSERT(alignment > 0);
	TRIVIAL_ASSERT((alignment & (alignment - 1)) == 0);
	TRIVIAL_ASSERT(alignment % pageSize() == 0);
	TRIVIAL_ASSERT(alignment % allocationGranularity() == 0);
	TRIVIAL_ASSERT(bytes % alignment == 0);

	if (bytes == 0 || bytes > SIZE_MAX - alignment) {
		outOsErrorCode = 0;
		return nullptr;
	}

	const std::size_t kRawBytes = bytes + alignment;
	void* raw = mmap(nullptr, kRawBytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if (raw == MAP_FAILED) {
		outOsErrorCode = errno;
		return nullptr;
	}

	const std::size_t kOffset = alignmentOffset(raw, alignment);

	char* aligned = static_cast<char*>(raw) + kOffset; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

	if (kOffset > 0) {
		(void)munmap(raw, kOffset);
	}

	if (const std::size_t kTail = kRawBytes - kOffset - bytes; kTail > 0) {
		(void)munmap(aligned + bytes, kTail); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	}

	return aligned;
}

void releaseReservation(void* base, std::size_t bytes) noexcept {
	if (munmap(base, bytes) != 0) {
		TRIVIAL_LOG_ERROR_PREFIX("VirtualMemory", "reservation release failed (munmap)");
	}
}

[[nodiscard]] bool commitPages(void* addr, std::size_t bytes, std::size_t pageSize, int& outOsErrorCode) noexcept {
	TRIVIAL_ASSERT(addr != nullptr);
	TRIVIAL_ASSERT(bytes > 0);
	TRIVIAL_ASSERT(bytes % pageSize == 0);
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	TRIVIAL_ASSERT(reinterpret_cast<std::uintptr_t>(addr) % pageSize == 0);
	(void)pageSize;

	if (mprotect(addr, bytes, PROT_READ | PROT_WRITE) != 0) {
		outOsErrorCode = errno;
		return false;
	}

	if (madvise(addr, bytes, MADV_FREE_REUSE) != 0) {
		outOsErrorCode = errno;
		return false;
	}

	return true;
}

#if TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
bool commitLargePages(void* addr, std::size_t bytes, std::size_t largePageSize, int& outOsErrorCode) noexcept {
	TRIVIAL_ASSERT(addr != nullptr);
	TRIVIAL_ASSERT(largePageSize > 0);
	TRIVIAL_ASSERT(bytes % largePageSize == 0);
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	TRIVIAL_ASSERT(reinterpret_cast<std::uintptr_t>(addr) % largePageSize == 0);

	if (mprotect(addr, bytes, PROT_READ | PROT_WRITE) != 0) {
		outOsErrorCode = errno;
		return false;
	}

	return true;
}
#endif // TRIVIAL_MEMORY_ENABLE_LARGE_PAGES

#if TRIVIAL_MEMORY_ENABLE_DECOMMIT
void decommitPages(void* addr, std::size_t bytes, std::size_t pageSize) noexcept {
	TRIVIAL_ASSERT(addr != nullptr);
	TRIVIAL_ASSERT(bytes > 0);
	TRIVIAL_ASSERT(bytes % pageSize == 0);
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	TRIVIAL_ASSERT(reinterpret_cast<std::uintptr_t>(addr) % pageSize == 0);
	(void)pageSize;

#if TRIVIAL_MEMORY_LAZY_DECOMMIT
	constexpr int kAdvice = MADV_FREE;
#else
	// MADV_DONTNEED does not reliably release pages on Darwin,
	// MADV_FREE_REUSABLE releases eagerly and updates process accounting
	constexpr int kAdvice = MADV_FREE_REUSABLE;
#endif // Decommit advice

	TRIVIAL_VERIFY(madvise(addr, bytes, kAdvice) == 0);
	TRIVIAL_VERIFY(mprotect(addr, bytes, PROT_NONE) == 0);
}
#endif // TRIVIAL_MEMORY_ENABLE_DECOMMIT

} // namespace trivial::memory

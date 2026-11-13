#define TRIVIAL_MEMORY_OS_MEMORY_IMPLEMENTATION
#include "core/memory/os/os_memory.h"

#if TRIVIAL_MEMORY_LAZY_DECOMMIT
#include <atomic>
#endif // TRIVIAL_MEMORY_LAZY_DECOMMIT
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/sysinfo.h>
#include <unistd.h>

#include <trivial/core/assert.h>
#include <trivial/core/log.h>
#include <trivial/core/memory/alignment.h>
#include <trivial/core/memory/byte_count.h>
#include <trivial/core/memory/memory_config.h>
#include <trivial/core/memory/pointer_arithmetic.h>

#if TRIVIAL_ENABLE_ASSERTS
#include <trivial/core/memory/memory.h>
#endif // TRIVIAL_ENABLE_ASSERTS

namespace {

#if TRIVIAL_MEMORY_LAZY_DECOMMIT
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
constinit std::atomic<bool> g_lazyDecommitUnavailable = false;
#endif // TRIVIAL_MEMORY_LAZY_DECOMMIT

[[nodiscard]] void* mapReservation(trivial::memory::ByteCount size) noexcept {
	void* const kMapping
	    = mmap(nullptr, size.toSize(), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
	return kMapping == MAP_FAILED ? nullptr : kMapping;
}

} // namespace

namespace trivial::memory {

[[nodiscard]] PageInfo probePageInfo() noexcept {
	const long kOsPageSize = sysconf(_SC_PAGESIZE);
	if (kOsPageSize <= 0) [[unlikely]] {
		TRIVIAL_LOG_FATAL_PREFIX("Memory",
		                         "OS page size probe failed, consider a 4096 byte default if this needs to run anyway");
		std::abort();
	}

	const ByteCount kPageSize = bytes(static_cast<std::uint64_t>(kOsPageSize));

	return {
	    .osPageSize = kPageSize,
	    .osAllocationGranularity = kPageSize,
	};
}

[[nodiscard]] ByteCount probePhysicalMemorySize() noexcept {
	struct sysinfo systemInfo{};
	std::uint64_t physicalMemory = 0;
	if (sysinfo(&systemInfo) != 0 || systemInfo.mem_unit == 0
	    || __builtin_mul_overflow(systemInfo.totalram, systemInfo.mem_unit, &physicalMemory)) {
		return {};
	}

	return bytes(physicalMemory);
}

[[nodiscard]] void* reserveAligned(ByteCount size, ByteCount alignment, int& outOsErrorCode) noexcept {
	TRIVIAL_ASSERT(alignment.isPowerOfTwo());
	TRIVIAL_ASSERT(isAligned(alignment, osPageSize()));
	TRIVIAL_ASSERT(isAligned(alignment, osAllocationGranularity()));
	TRIVIAL_ASSERT(isAligned(size, alignment));

	if (size.isZero() || size.count() > SIZE_MAX - alignment.count()) {
		outOsErrorCode = 0;
		return nullptr;
	}

	void* aligned = mapReservation(size);

	if (aligned != nullptr && !isAligned(aligned, alignment)) {
		TRIVIAL_VERIFY(munmap(aligned, size.toSize()) == 0);
		aligned = nullptr;
	}

	if (aligned == nullptr) {
		const ByteCount kRawSize = size + alignment;
		void* raw = mapReservation(kRawSize);

		if (raw == nullptr) {
			outOsErrorCode = errno;
			return nullptr;
		}

		aligned = alignUp(raw, alignment);
		const ByteCount kOffset = pointerDifference(aligned, raw);

		if (!kOffset.isZero()) {
			TRIVIAL_VERIFY(munmap(raw, kOffset.toSize()) == 0);
		}

		if (const ByteCount kTail = kRawSize - kOffset - size; !kTail.isZero()) {
			TRIVIAL_VERIFY(munmap(pointerPlus(aligned, size), kTail.toSize()) == 0);
		}
	}

#if defined(PR_SET_VMA) && defined(PR_SET_VMA_ANON_NAME)
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
	(void)prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME, aligned, size.toSize(), "trivial");
#endif // PR_SET_VMA

	return aligned;
}

void releaseReservation(void* base, ByteCount size) noexcept {
	if (munmap(base, size.toSize()) != 0) {
		TRIVIAL_LOG_ERROR_PREFIX("OsMemory", "reservation release failed (munmap)");
	}
}

[[nodiscard]] bool commitPages(void* addr, ByteCount size, int& outOsErrorCode) noexcept {
	TRIVIAL_ASSERT(addr != nullptr);
	TRIVIAL_ASSERT(!size.isZero());
	TRIVIAL_ASSERT(isAligned(size, osPageSize()));
	TRIVIAL_ASSERT(isAligned(addr, osPageSize()));
	(void)addr;
	(void)size;
	outOsErrorCode = 0;

	return true;
}

#if TRIVIAL_MEMORY_ENABLE_DECOMMIT
void decommitPages(void* addr, ByteCount size) noexcept {
	TRIVIAL_ASSERT(addr != nullptr);
	TRIVIAL_ASSERT(!size.isZero());
	TRIVIAL_ASSERT(isAligned(size, osPageSize()));
	TRIVIAL_ASSERT(isAligned(addr, osPageSize()));

	TRIVIAL_VERIFY(madvise(addr, size.toSize(), MADV_DONTNEED) == 0);
}

#if TRIVIAL_MEMORY_LAZY_DECOMMIT
void lazyDecommitPages(void* addr, ByteCount size) noexcept {
	TRIVIAL_ASSERT(addr != nullptr);
	TRIVIAL_ASSERT(!size.isZero());
	TRIVIAL_ASSERT(isAligned(size, osPageSize()));
	TRIVIAL_ASSERT(isAligned(addr, osPageSize()));

	if (!g_lazyDecommitUnavailable.load(std::memory_order_relaxed)) {
		int result = madvise(addr, size.toSize(), MADV_FREE);
		for (std::size_t i = 0; result != 0 && errno == EAGAIN && i < TRIVIAL_MEMORY_LAZY_DECOMMIT_MAX_RETRIES; ++i) {
			result = madvise(addr, size.toSize(), MADV_FREE);
		}

		if (result == 0) {
			return;
		}

		if (errno == EINVAL) {
			g_lazyDecommitUnavailable.store(true, std::memory_order_release);
		}
	}

	decommitPages(addr, size);
}
#endif // TRIVIAL_MEMORY_LAZY_DECOMMIT
#endif // TRIVIAL_MEMORY_ENABLE_DECOMMIT

[[nodiscard]] bool makePagesNoAccess(void* addr, ByteCount size, int& outOsErrorCode) noexcept {
	TRIVIAL_ASSERT(addr != nullptr);
	TRIVIAL_ASSERT(!size.isZero());
	TRIVIAL_ASSERT(isAligned(size, trivial::memory::osPageSize()));
	TRIVIAL_ASSERT(isAligned(addr, trivial::memory::osPageSize()));

	if (mprotect(addr, size.toSize(), PROT_NONE) != 0) {
		outOsErrorCode = errno;
		return false;
	}

	return true;
}

[[nodiscard]] bool makePagesReadOnly(void* addr, ByteCount size, int& outOsErrorCode) noexcept {
	TRIVIAL_ASSERT(addr != nullptr);
	TRIVIAL_ASSERT(!size.isZero());
	TRIVIAL_ASSERT(isAligned(size, trivial::memory::osPageSize()));
	TRIVIAL_ASSERT(isAligned(addr, trivial::memory::osPageSize()));

	if (mprotect(addr, size.toSize(), PROT_READ) != 0) {
		outOsErrorCode = errno;
		return false;
	}

	return true;
}

[[nodiscard]] bool makePagesReadWrite(void* addr, ByteCount size, int& outOsErrorCode) noexcept {
	TRIVIAL_ASSERT(addr != nullptr);
	TRIVIAL_ASSERT(!size.isZero());
	TRIVIAL_ASSERT(isAligned(size, trivial::memory::osPageSize()));
	TRIVIAL_ASSERT(isAligned(addr, trivial::memory::osPageSize()));

	if (mprotect(addr, size.toSize(), PROT_READ | PROT_WRITE) != 0) {
		outOsErrorCode = errno;
		return false;
	}

	return true;
}

} // namespace trivial::memory

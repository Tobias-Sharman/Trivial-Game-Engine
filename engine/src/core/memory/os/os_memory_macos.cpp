#define TRIVIAL_MEMORY_OS_MEMORY_IMPLEMENTATION
#include "core/memory/os/os_memory.h"

#include <array>
#include <bit>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/mach_init.h>
#include <mach/mach_vm.h>
#include <mach/memory_object_types.h>
#include <mach/vm_inherit.h>
#include <mach/vm_prot.h>
#include <mach/vm_statistics.h>
#include <sys/mman.h>
#include <sys/sysctl.h>
#include <unistd.h>

#include <trivial/core/assert.h>
#include <trivial/core/log.h>
#include <trivial/core/memory/byte_count.h>
#include <trivial/core/memory/memory_config.h>

#if TRIVIAL_ENABLE_ASSERTS
#include <trivial/core/memory/alignment.h>
#include <trivial/core/memory/memory.h>
#endif // TRIVIAL_ENABLE_ASSERTS

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
	std::array<int, 2> name = {CTL_HW, HW_MEMSIZE};
	std::int64_t physicalMemory = 0;
	std::size_t length = sizeof(physicalMemory);
	if (sysctl(name.data(), name.size(), &physicalMemory, &length, nullptr, 0) != 0 || physicalMemory <= 0) {
		return {};
	}

	return bytes(static_cast<std::uint64_t>(physicalMemory));
}

[[nodiscard]] void* reserveAligned(ByteCount size, ByteCount alignment, int& outOsErrorCode) noexcept {
	TRIVIAL_ASSERT(alignment.isPowerOfTwo());
	TRIVIAL_ASSERT(isAligned(alignment, osPageSize()));
	TRIVIAL_ASSERT(isAligned(alignment, osAllocationGranularity()));
	TRIVIAL_ASSERT(isAligned(size, alignment));

	if (size.isZero()) [[unlikely]] {
		outOsErrorCode = 0;
		return nullptr;
	}

	mach_vm_address_t address = 0;
	const kern_return_t kResult = mach_vm_map(mach_task_self(),
	                                          &address,
	                                          size.count(),
	                                          alignment.count() - 1,
	                                          VM_FLAGS_ANYWHERE | VM_MAKE_TAG(VM_MEMORY_APPLICATION_SPECIFIC_16),
	                                          MEMORY_OBJECT_NULL,
	                                          0,
	                                          FALSE,
	                                          VM_PROT_READ | VM_PROT_WRITE,
	                                          VM_PROT_READ | VM_PROT_WRITE,
	                                          VM_INHERIT_DEFAULT);

	if (kResult != KERN_SUCCESS) {
		outOsErrorCode = kResult;
		return nullptr;
	}

	return std::bit_cast<void*>(address);
}

void releaseReservation(void* base, ByteCount size) noexcept {
	if (mach_vm_deallocate(mach_task_self(), std::bit_cast<mach_vm_address_t>(base), size.count()) != KERN_SUCCESS) {
		TRIVIAL_LOG_ERROR_PREFIX("OsMemory", "reservation release failed (mach_vm_deallocate)");
	}
}

[[nodiscard]] bool commitPages(void* addr, ByteCount size, int& outOsErrorCode) noexcept {
	TRIVIAL_ASSERT(addr != nullptr);
	TRIVIAL_ASSERT(!size.isZero());
	TRIVIAL_ASSERT(isAligned(size, osPageSize()));
	TRIVIAL_ASSERT(isAligned(addr, osPageSize()));
	outOsErrorCode = 0;

	if (madvise(addr, size.toSize(), MADV_FREE_REUSE) != 0) {
		TRIVIAL_LOG_WARNING_PREFIX("OsMemory", "reuse advice failed (MADV_FREE_REUSE)");
	}

	return true;
}

#if TRIVIAL_MEMORY_ENABLE_DECOMMIT
void decommitPages(void* addr, ByteCount size) noexcept {
	TRIVIAL_ASSERT(addr != nullptr);
	TRIVIAL_ASSERT(!size.isZero());
	TRIVIAL_ASSERT(isAligned(size, osPageSize()));
	TRIVIAL_ASSERT(isAligned(addr, osPageSize()));

	// MADV_DONTNEED does not reliably release pages on Darwin,
	// MADV_FREE_REUSABLE releases eagerly and updates process accounting
	if (madvise(addr, size.toSize(), MADV_FREE_REUSABLE) != 0) {
		TRIVIAL_VERIFY(madvise(addr, size.toSize(), MADV_DONTNEED) == 0);
	}
}

#if TRIVIAL_MEMORY_LAZY_DECOMMIT
void lazyDecommitPages(void* addr, ByteCount size) noexcept {
	TRIVIAL_ASSERT(addr != nullptr);
	TRIVIAL_ASSERT(!size.isZero());
	TRIVIAL_ASSERT(isAligned(size, osPageSize()));
	TRIVIAL_ASSERT(isAligned(addr, osPageSize()));

	int result = madvise(addr, size.toSize(), MADV_FREE);
	for (std::size_t i = 0; result != 0 && errno == EAGAIN && i < TRIVIAL_MEMORY_LAZY_DECOMMIT_MAX_RETRIES; ++i) {
		result = madvise(addr, size.toSize(), MADV_FREE);
	}

	if (result == 0) {
		return;
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

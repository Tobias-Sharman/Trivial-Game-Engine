#include "core/memory/os/os_memory.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX

#include <windows.h> // IWYU pragma: keep

#include <trivial/core/assert.h>
#include <trivial/core/log.h>
#include <trivial/core/memory/byte_count.h>
#include <trivial/core/memory/memory_config.h>

#if TRIVIAL_ENABLE_ASSERTS
#include <trivial/core/memory/alignment.h>
#include <trivial/core/memory/memory.h>
#include <trivial/core/memory/pointer_arithmetic.h>
#endif // TRIVIAL_ENABLE_ASSERTS

namespace trivial::memory {

[[nodiscard]] PageInfo probePageInfo() noexcept {
	SYSTEM_INFO systemInfo;
	GetNativeSystemInfo(&systemInfo);

	return {
	    .osPageSize = bytes(systemInfo.dwPageSize),
	    .osAllocationGranularity = bytes(systemInfo.dwAllocationGranularity),
	};
}

[[nodiscard]] ByteCount probePhysicalMemorySize() noexcept {
	ULONGLONG physicalMemoryKiB = 0;
	if (GetPhysicallyInstalledSystemMemory(&physicalMemoryKiB) == FALSE) {
		return {};
	}

	return kibibytes(physicalMemoryKiB);
}

[[nodiscard]] void* reserveAligned(ByteCount size, ByteCount alignment, int& outOsErrorCode) noexcept {
	TRIVIAL_ASSERT(alignment.isPowerOfTwo());
	TRIVIAL_ASSERT(isAligned(alignment, osPageSize()));
	TRIVIAL_ASSERT(isAligned(alignment, osAllocationGranularity()));
	TRIVIAL_ASSERT(isAligned(size, alignment));

	if (size.isZero()) {
		outOsErrorCode = 0;
		return nullptr;
	}

	MEM_ADDRESS_REQUIREMENTS requirements{.Alignment = alignment.toSize()};

	MEM_EXTENDED_PARAMETER parameter{.Type = MemExtendedParameterAddressRequirements, .Pointer = &requirements};

	void* result
	    = VirtualAlloc2(GetCurrentProcess(), nullptr, size.toSize(), MEM_RESERVE, PAGE_NOACCESS, &parameter, 1);

	if (result == nullptr) {
		outOsErrorCode = static_cast<int>(GetLastError());
	}

	return result;
}

void releaseReservation(void* base, ByteCount size) noexcept {
	(void)size;
	if (VirtualFree(base, 0, MEM_RELEASE) == 0) {
		TRIVIAL_LOG_ERROR_PREFIX("OsMemory", "reservation release failed (VirtualFree)");
	}
}

[[nodiscard]] bool commitPages(void* addr, ByteCount size, int& outOsErrorCode) noexcept {
	TRIVIAL_ASSERT(addr != nullptr);
	TRIVIAL_ASSERT(!size.isZero());
	TRIVIAL_ASSERT(isAligned(size, osPageSize()));
	TRIVIAL_ASSERT(isAligned(addr, osPageSize()));

	if (VirtualAlloc(addr, size.toSize(), MEM_COMMIT, PAGE_READWRITE) == nullptr) {
		outOsErrorCode = static_cast<int>(GetLastError());
		return false;
	}

	return true;
}

#if TRIVIAL_MEMORY_ENABLE_DECOMMIT
void decommitPages(void* addr, ByteCount size) noexcept {
	TRIVIAL_ASSERT(addr != nullptr);
	TRIVIAL_ASSERT(!size.isZero());
	TRIVIAL_ASSERT(isAligned(size, osPageSize()));
	TRIVIAL_ASSERT(isAligned(addr, osPageSize()));

	TRIVIAL_VERIFY(VirtualFree(addr, size.toSize(), MEM_DECOMMIT) != 0);
}

#if TRIVIAL_MEMORY_LAZY_DECOMMIT
void lazyDecommitPages(void* addr, ByteCount size) noexcept {
	TRIVIAL_ASSERT(addr != nullptr);
	TRIVIAL_ASSERT(!size.isZero());
	TRIVIAL_ASSERT(isAligned(size, osPageSize()));
	TRIVIAL_ASSERT(isAligned(addr, osPageSize()));

	if (VirtualAlloc(addr, size.toSize(), MEM_RESET, PAGE_READWRITE) != nullptr) {
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

	DWORD oldProtection = 0;
	if (VirtualProtect(addr, size.toSize(), PAGE_NOACCESS, &oldProtection) == 0) {
		outOsErrorCode = static_cast<int>(GetLastError());
		return false;
	}

	return true;
}

[[nodiscard]] bool makePagesReadOnly(void* addr, ByteCount size, int& outOsErrorCode) noexcept {
	TRIVIAL_ASSERT(addr != nullptr);
	TRIVIAL_ASSERT(!size.isZero());
	TRIVIAL_ASSERT(isAligned(size, trivial::memory::osPageSize()));
	TRIVIAL_ASSERT(isAligned(addr, trivial::memory::osPageSize()));

	DWORD oldProtection = 0;
	if (VirtualProtect(addr, size.toSize(), PAGE_READONLY, &oldProtection) == 0) {
		outOsErrorCode = static_cast<int>(GetLastError());
		return false;
	}

	return true;
}

[[nodiscard]] bool makePagesReadWrite(void* addr, ByteCount size, int& outOsErrorCode) noexcept {
	TRIVIAL_ASSERT(addr != nullptr);
	TRIVIAL_ASSERT(!size.isZero());
	TRIVIAL_ASSERT(isAligned(size, trivial::memory::osPageSize()));
	TRIVIAL_ASSERT(isAligned(addr, trivial::memory::osPageSize()));

	DWORD oldProtection = 0;
	if (VirtualProtect(addr, size.toSize(), PAGE_READWRITE, &oldProtection) == 0) {
		outOsErrorCode = static_cast<int>(GetLastError());
		return false;
	}

	return true;
}

} // namespace trivial::memory

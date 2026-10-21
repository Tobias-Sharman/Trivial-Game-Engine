#include "core/memory/virtual_memory.h"

#include <cstddef>

#if TRIVIAL_ENABLE_ASSERTS
#include <cstdint>
#endif // TRIVIAL_ENABLE_ASSERTS

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX

#include <windows.h> // IWYU pragma: keep

#include <trivial/core/assert.h>
#include <trivial/core/log.h>
#include <trivial/core/memory/memory_config.h>

#if TRIVIAL_ENABLE_ASSERTS
#include <trivial/core/memory/memory.h>
#endif // TRIVIAL_ENABLE_ASSERTS

namespace trivial::memory {

#if TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
bool adjustLockMemoryPrivilege(bool enable) noexcept {
	HANDLE token = nullptr;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &token) == 0) {
		return false;
	}

	LUID luid;
	bool changed = false;

	if (LookupPrivilegeValueW(nullptr, SE_LOCK_MEMORY_NAME, &luid) != 0) {
		TOKEN_PRIVILEGES privileges{};
		privileges.PrivilegeCount = 1;
		privileges.Privileges[0].Luid = luid;
		privileges.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;

		changed = AdjustTokenPrivileges(token, FALSE, &privileges, 0, nullptr, nullptr) != 0
		          && GetLastError() == ERROR_SUCCESS;
	}

	CloseHandle(token);
	return changed;
}
#endif // TRIVIAL_MEMORY_ENABLE_LARGE_PAGES

[[nodiscard]] SystemInfo probeSystemInfo() noexcept {
	SystemInfo info;

	SYSTEM_INFO systemInfo;
	GetNativeSystemInfo(&systemInfo);

	info.pageSize = static_cast<std::size_t>(systemInfo.dwPageSize);
	info.allocationGranularity = static_cast<std::size_t>(systemInfo.dwAllocationGranularity);

#if TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
	info.largePageSize = static_cast<std::size_t>(GetLargePageMinimum());
#endif // TRIVIAL_MEMORY_ENABLE_LARGE_PAGES

	return info;
}

[[nodiscard]] void* reserveAligned(std::size_t bytes, std::size_t alignment, int& outOsErrorCode) noexcept {
	TRIVIAL_ASSERT(alignment > 0);
	TRIVIAL_ASSERT((alignment & (alignment - 1)) == 0);
	TRIVIAL_ASSERT(alignment % pageSize() == 0);
	TRIVIAL_ASSERT(alignment % allocationGranularity() == 0);
	TRIVIAL_ASSERT(bytes % alignment == 0);

	if (bytes == 0) {
		outOsErrorCode = 0;
		return nullptr;
	}

	MEM_ADDRESS_REQUIREMENTS requirements{.Alignment = alignment};

	MEM_EXTENDED_PARAMETER parameter{.Type = MemExtendedParameterAddressRequirements, .Pointer = &requirements};

	void* result = VirtualAlloc2(GetCurrentProcess(), nullptr, bytes, MEM_RESERVE, PAGE_NOACCESS, &parameter, 1);

	if (result == nullptr) {
		outOsErrorCode = static_cast<int>(GetLastError());
	}

	return result;
}

void releaseReservation(void* base, std::size_t bytes) noexcept {
	(void)bytes;

	if (VirtualFree(base, 0, MEM_RELEASE) == 0) {
		TRIVIAL_LOG_ERROR_PREFIX("VirtualMemory", "reservation release failed (VirtualFree)");
	}
}

[[nodiscard]] bool commitPages(void* addr, std::size_t bytes, std::size_t pageSize, int& outOsErrorCode) noexcept {
	TRIVIAL_ASSERT(addr != nullptr);
	TRIVIAL_ASSERT(bytes > 0);
	TRIVIAL_ASSERT(bytes % pageSize == 0);
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	TRIVIAL_ASSERT(reinterpret_cast<std::uintptr_t>(addr) % pageSize == 0);
	(void)pageSize;

	if (VirtualAlloc(addr, bytes, MEM_COMMIT, PAGE_READWRITE) == nullptr) {
		outOsErrorCode = static_cast<int>(GetLastError());
		return false;
	}

	if constexpr (TRIVIAL_MEMORY_LAZY_DECOMMIT) {
		DWORD oldProtect = 0;
		if (VirtualProtect(addr, bytes, PAGE_READWRITE, &oldProtect) == 0) {
			outOsErrorCode = static_cast<int>(GetLastError());
			return false;
		}
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

	(void)largePageSize;

	if (VirtualAlloc(addr, bytes, MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE) == nullptr) {
		outOsErrorCode = static_cast<int>(GetLastError());
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

	if constexpr (TRIVIAL_MEMORY_LAZY_DECOMMIT) {
		if (VirtualAlloc(addr, bytes, MEM_RESET, PAGE_READWRITE) != nullptr) {
			DWORD oldProtect = 0;
			TRIVIAL_VERIFY(VirtualProtect(addr, bytes, PAGE_NOACCESS, &oldProtect) != 0);

			return;
		}
	}

	TRIVIAL_VERIFY(VirtualFree(addr, bytes, MEM_DECOMMIT) != 0);
}
#endif // TRIVIAL_MEMORY_ENABLE_DECOMMIT

} // namespace trivial::memory

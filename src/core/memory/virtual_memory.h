#ifndef TRIVIAL_SRC_CORE_MEMORY_VIRTUAL_MEMORY_H
#define TRIVIAL_SRC_CORE_MEMORY_VIRTUAL_MEMORY_H

#include <cstddef>
#include <cstdint>

#include <trivial/core/assert.h>
#include <trivial/core/log.h>
#include <trivial/core/memory/memory_config.h>
#include <trivial/core/platform.h>

#if TRIVIAL_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#elif TRIVIAL_PLATFORM_POSIX
#include <cerrno>
#include <sys/mman.h>
#include <unistd.h>

#else
#error "Unsupported platform in virtual_memory.h"

#endif // Platform check

namespace trivial::memory {

struct SystemInfo {
	std::size_t pageSize = 0;
	std::size_t allocationGranularity = 0;
#if TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
	std::size_t largePageSize = 0;
#endif // TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
};

#if TRIVIAL_PLATFORM_WINDOWS
using VirtualAlloc2Fn = PVOID(WINAPI*)(HANDLE, PVOID, SIZE_T, ULONG, ULONG, MEM_EXTENDED_PARAMETER*, ULONG);

inline VirtualAlloc2Fn resolveVirtualAlloc2() noexcept {
#if TRIVIAL_PLATFORM_SDK_HAS_VIRTUAL_ALLOC2
	HMODULE module = GetModuleHandleW(L"kernelbase.dll");
	if (module == nullptr) {
		return nullptr;
	}

	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	return reinterpret_cast<VirtualAlloc2Fn>(GetProcAddress(module, "VirtualAlloc2"));
#else
	return nullptr;
#endif // TRIVIAL_PLATFORM_SDK_HAS_VIRTUAL_ALLOC2
}

inline VirtualAlloc2Fn virtualAlloc2() noexcept {
	static VirtualAlloc2Fn function = resolveVirtualAlloc2();
	return function;
}
#endif // TRIVIAL_PLATFORM_WINDOWS

#if TRIVIAL_MEMORY_ENABLE_LARGE_PAGES && TRIVIAL_PLATFORM_WINDOWS
inline bool adjustLockMemoryPrivilege(bool enable) noexcept {
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
#endif // TRIVIAL_MEMORY_ENABLE_LARGE_PAGES && TRIVIAL_PLATFORM_WINDOWS

inline SystemInfo probeSystemInfo() noexcept {
	SystemInfo info;

#if TRIVIAL_PLATFORM_WINDOWS
	SYSTEM_INFO systemInfo;
	GetNativeSystemInfo(&systemInfo);

	info.pageSize = static_cast<std::size_t>(systemInfo.dwPageSize);
	info.allocationGranularity = static_cast<std::size_t>(systemInfo.dwAllocationGranularity);

#if TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
	info.largePageSize = static_cast<std::size_t>(GetLargePageMinimum());
#endif // TRIVIAL_MEMORY_ENABLE_LARGE_PAGES

#elif TRIVIAL_PLATFORM_POSIX
	long pageSize = sysconf(_SC_PAGESIZE);
	TRIVIAL_ASSERT(pageSize > 0);

	info.pageSize = static_cast<std::size_t>(pageSize);
	info.allocationGranularity = info.pageSize;

#if TRIVIAL_MEMORY_ENABLE_LARGE_PAGES && TRIVIAL_PLATFORM_LINUX
	// TODO: need to verify this approach
	info.largePageSize = info.pageSize * (info.pageSize / sizeof(std::uintptr_t));
#endif // TRIVIAL_MEMORY_ENABLE_LARGE_PAGES && TRIVIAL_PLATFORM_LINUX

#endif // Platform check

	return info;
}

#if TRIVIAL_PLATFORM_POSIX
// MADV_FREE is declared on Linux and macOS but only implemented from Linux 4.5,
// so availability is decided by trying it once on a scratch page
inline bool probeMadvFree(std::size_t pageSize) noexcept {
#if TRIVIAL_PLATFORM_SDK_HAS_MADV_FREE
	void* probe = mmap(nullptr, pageSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (probe == MAP_FAILED) {
		return false;
	}

	bool supported = madvise(probe, pageSize, MADV_FREE) == 0;
	(void)munmap(probe, pageSize);

	return supported;
#else
	(void)pageSize;
	return false;
#endif // TRIVIAL_PLATFORM_SDK_HAS_MADV_FREE
}
#endif // TRIVIAL_PLATFORM_POSIX

inline std::size_t alignmentOffset(const void* base, std::size_t alignment) noexcept {
	TRIVIAL_ASSERT(alignment > 0);
	TRIVIAL_ASSERT((alignment & (alignment - 1)) == 0);

	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	std::uintptr_t misalignment = reinterpret_cast<std::uintptr_t>(base) & (alignment - 1);
	return misalignment == 0 ? 0 : alignment - static_cast<std::size_t>(misalignment);
}

#if TRIVIAL_PLATFORM_WINDOWS
inline void* reserveAligned(std::size_t bytes,
                            std::size_t alignment,
                            const SystemInfo& systemInfo,
                            void*& outRawBase,
                            std::size_t& outRawBytes,
                            int& outOsErrorCode) noexcept {
	TRIVIAL_ASSERT(alignment > 0);
	TRIVIAL_ASSERT((alignment & (alignment - 1)) == 0);
	TRIVIAL_ASSERT(alignment % systemInfo.pageSize == 0);
	TRIVIAL_ASSERT(alignment % systemInfo.allocationGranularity == 0);
	TRIVIAL_ASSERT(bytes % alignment == 0);

	if (bytes == 0 || bytes > SIZE_MAX - alignment) {
		outOsErrorCode = 0;
		return nullptr;
	}

	VirtualAlloc2Fn alloc2 = virtualAlloc2();

	if (alloc2 != nullptr) {
		MEM_ADDRESS_REQUIREMENTS requirements{};
		requirements.Alignment = alignment;

		MEM_EXTENDED_PARAMETER parameter{};
		parameter.Type = MemExtendedParameterAddressRequirements;
		parameter.Pointer = &requirements;

		void* result = alloc2(GetCurrentProcess(), nullptr, bytes, MEM_RESERVE, PAGE_NOACCESS, &parameter, 1);

		if (result != nullptr) {
			outRawBase = result;
			outRawBytes = bytes;
			return result;
		}
	}

	std::size_t rawBytes = bytes + alignment;
	void* raw = VirtualAlloc(nullptr, rawBytes, MEM_RESERVE, PAGE_NOACCESS);

	if (raw == nullptr) {
		outOsErrorCode = static_cast<int>(GetLastError());
		return nullptr;
	}

	outRawBase = raw;
	outRawBytes = rawBytes;

	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	return static_cast<char*>(raw) + alignmentOffset(raw, alignment);
}

#elif TRIVIAL_PLATFORM_POSIX
inline void* reserveAligned(std::size_t bytes,
                            std::size_t alignment,
                            const SystemInfo& systemInfo,
                            int& outOsErrorCode) noexcept {
	TRIVIAL_ASSERT(alignment > 0);
	TRIVIAL_ASSERT((alignment & (alignment - 1)) == 0);
	TRIVIAL_ASSERT(alignment % systemInfo.pageSize == 0);
	TRIVIAL_ASSERT(bytes % alignment == 0);
	(void)systemInfo;

	if (bytes == 0 || bytes > SIZE_MAX - alignment) {
		outOsErrorCode = 0;
		return nullptr;
	}

	std::size_t rawBytes = bytes + alignment;
	void* raw = mmap(nullptr, rawBytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if (raw == MAP_FAILED) {
		outOsErrorCode = errno;
		return nullptr;
	}

	std::size_t offset = alignmentOffset(raw, alignment);

	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	char* aligned = static_cast<char*>(raw) + offset;

	if (offset > 0) {
		(void)munmap(raw, offset);
	}

	std::size_t tail = rawBytes - offset - bytes;
	if (tail > 0) {
		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
		(void)munmap(aligned + bytes, tail);
	}

	return aligned;
}
#endif // Platform check

inline void releaseReservation(void* base, std::size_t bytes) noexcept {
#if TRIVIAL_PLATFORM_WINDOWS
	(void)bytes;

	if (VirtualFree(base, 0, MEM_RELEASE) == 0) {
		TRIVIAL_LOG_ERROR_PREFIX("VirtualMemory", "reservation release failed (VirtualFree)");
	}

#elif TRIVIAL_PLATFORM_POSIX
	if (munmap(base, bytes) != 0) {
		TRIVIAL_LOG_ERROR_PREFIX("VirtualMemory", "reservation release failed (munmap)");
	}

#endif // Platform check
}

inline bool commitPages(void* addr, std::size_t bytes, std::size_t pageSize, int& outOsErrorCode) noexcept {
	TRIVIAL_ASSERT(addr != nullptr);
	TRIVIAL_ASSERT(bytes > 0);
	TRIVIAL_ASSERT(bytes % pageSize == 0);
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	TRIVIAL_ASSERT(reinterpret_cast<std::uintptr_t>(addr) % pageSize == 0);
	(void)pageSize;

#if TRIVIAL_PLATFORM_WINDOWS
	if (VirtualAlloc(addr, bytes, MEM_COMMIT, PAGE_READWRITE) == nullptr) {
		outOsErrorCode = static_cast<int>(GetLastError());
		return false;
	}

#elif TRIVIAL_PLATFORM_POSIX
	if (mprotect(addr, bytes, PROT_READ | PROT_WRITE) != 0) {
		outOsErrorCode = errno;
		return false;
	}

#endif // Platform check

	return true;
}

#if TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
inline bool commitLargePages(void* addr, std::size_t bytes, std::size_t largePageSize, int& outOsErrorCode) noexcept {
	TRIVIAL_ASSERT(addr != nullptr);
	TRIVIAL_ASSERT(largePageSize > 0);
	TRIVIAL_ASSERT(bytes % largePageSize == 0);
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	TRIVIAL_ASSERT(reinterpret_cast<std::uintptr_t>(addr) % largePageSize == 0);

#if TRIVIAL_PLATFORM_WINDOWS
	(void)largePageSize;

	if (VirtualAlloc(addr, bytes, MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE) == nullptr) {
		outOsErrorCode = static_cast<int>(GetLastError());
		return false;
	}

#elif TRIVIAL_PLATFORM_POSIX
	if (mprotect(addr, bytes, PROT_READ | PROT_WRITE) != 0) {
		outOsErrorCode = errno;
		return false;
	}

#if TRIVIAL_PLATFORM_LINUX
	(void)madvise(addr, bytes, MADV_HUGEPAGE);
#endif // TRIVIAL_PLATFORM_LINUX

#endif // Platform check

	return true;
}
#endif // TRIVIAL_MEMORY_ENABLE_LARGE_PAGES

inline void decommitPages(void* addr,
                          std::size_t bytes,
                          std::size_t pageSize,
                          trivial::memory::DecommitMode mode) noexcept {
	if (mode == trivial::memory::DecommitMode::Disabled) {
		return;
	}

	TRIVIAL_ASSERT(addr != nullptr);
	TRIVIAL_ASSERT(bytes > 0);
	TRIVIAL_ASSERT(bytes % pageSize == 0);
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	TRIVIAL_ASSERT(reinterpret_cast<std::uintptr_t>(addr) % pageSize == 0);
	(void)pageSize;

#if TRIVIAL_PLATFORM_WINDOWS
	bool ok = VirtualFree(addr, bytes, MEM_DECOMMIT) != 0;
	TRIVIAL_ASSERT(ok);
	(void)ok;

#elif TRIVIAL_PLATFORM_POSIX
#if TRIVIAL_PLATFORM_MACOS
	// MADV_DONTNEED does not reliably release pages on Darwin
	// MADV_FREE_REUSABLE for eager release for updating process accounting
	// MADV_FREE defers reclaim to memory pressure
	int advice = mode == trivial::memory::DecommitMode::Lazy ? MADV_FREE : MADV_FREE_REUSABLE;
#elif TRIVIAL_PLATFORM_SDK_HAS_MADV_FREE
	int advice = mode == trivial::memory::DecommitMode::Lazy ? MADV_FREE : MADV_DONTNEED;
#else
	int advice = MADV_DONTNEED;
#endif // Decommit advice

	bool ok = madvise(addr, bytes, advice) == 0;
	TRIVIAL_ASSERT(ok);
	(void)ok;

	if (mprotect(addr, bytes, PROT_NONE) != 0) {
		TRIVIAL_ASSERT(false);
		return;
	}

#endif // Platform check
}

} // namespace trivial::memory

#endif // TRIVIAL_SRC_CORE_MEMORY_VIRTUAL_MEMORY_H

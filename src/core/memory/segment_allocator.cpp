#include <trivial/core/memory/segment_allocator.h>

#include <algorithm>
#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>

#include <trivial/core/assert.h>
#include <trivial/core/log.h>
#include <trivial/core/platform.h>
#include <trivial/core/profile.h>

#if TRIVIAL_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#elif TRIVIAL_PLATFORM_POSIX
#include <cerrno>
#include <sys/mman.h>
#include <unistd.h>

#else
#error "Unsupported platform in segment_allocator.cpp"

#endif // Platform check

// TODO: Some of these functions would for sure benefit from the safety gained
//       from using a named struct

namespace {

constexpr std::size_t g_kInvalidIndex = static_cast<std::size_t>(-1);

constexpr std::size_t g_kBitsPerWord = std::numeric_limits<std::uint64_t>::digits;
constexpr std::size_t g_kWordShift = std::countr_zero(g_kBitsPerWord);
constexpr std::size_t g_kBitIndexMask = g_kBitsPerWord - 1;
constexpr std::uint64_t g_kFullWord = ~std::uint64_t{0};

struct SystemInfo {
	std::size_t pageSize = 0;
	std::size_t allocationGranularity = 0;
#if TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
	std::size_t largePageSize = 0;
#endif // TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
};

#if TRIVIAL_PLATFORM_WINDOWS
using VirtualAlloc2Fn = PVOID(WINAPI*)(HANDLE, PVOID, SIZE_T, ULONG, ULONG, MEM_EXTENDED_PARAMETER*, ULONG);

VirtualAlloc2Fn resolveVirtualAlloc2() noexcept {
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

VirtualAlloc2Fn virtualAlloc2() noexcept {
	static VirtualAlloc2Fn function = resolveVirtualAlloc2();
	return function;
}
#endif // TRIVIAL_PLATFORM_WINDOWS

#if TRIVIAL_MEMORY_ENABLE_LARGE_PAGES && TRIVIAL_PLATFORM_WINDOWS
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
#endif // TRIVIAL_MEMORY_ENABLE_LARGE_PAGES && TRIVIAL_PLATFORM_WINDOWS

SystemInfo probeSystemInfo() noexcept {
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
bool probeMadvFree(std::size_t pageSize) noexcept {
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

std::size_t alignmentOffset(const void* base, std::size_t alignment) noexcept {
	TRIVIAL_ASSERT(alignment > 0);
	TRIVIAL_ASSERT((alignment & (alignment - 1)) == 0);

	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	std::uintptr_t misalignment = reinterpret_cast<std::uintptr_t>(base) & (alignment - 1);
	return misalignment == 0 ? 0 : alignment - static_cast<std::size_t>(misalignment);
}

#if TRIVIAL_PLATFORM_WINDOWS
void* reserveAligned(std::size_t bytes,
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
void* reserveAligned(std::size_t bytes,
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

void releaseReservation(void* base, std::size_t bytes) noexcept {
#if TRIVIAL_PLATFORM_WINDOWS
	(void)bytes;

	if (VirtualFree(base, 0, MEM_RELEASE) == 0) {
		TRIVIAL_LOG_ERROR_PREFIX("SegmentAllocator", "reservation release failed (VirtualFree)");
	}

#elif TRIVIAL_PLATFORM_POSIX
	if (munmap(base, bytes) != 0) {
		TRIVIAL_LOG_ERROR_PREFIX("SegmentAllocator", "reservation release failed (munmap)");
	}

#endif // Platform check
}

bool commitPages(void* addr, std::size_t bytes, std::size_t pageSize, int& outOsErrorCode) noexcept {
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
bool commitLargePages(void* addr, std::size_t bytes, std::size_t largePageSize, int& outOsErrorCode) noexcept {
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

void decommitPages(void* addr, std::size_t bytes, std::size_t pageSize, trivial::memory::DecommitMode mode) noexcept {
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

void* mapMetadata(std::size_t bytes, std::size_t pageSize, std::size_t& outMappingBytes, int& outOsErrorCode) noexcept {
	std::size_t payload = (bytes + pageSize - 1) & ~(pageSize - 1);

	if (payload == 0 || payload > SIZE_MAX - (2 * pageSize)) {
		outOsErrorCode = 0;
		return nullptr;
	}

	std::size_t mappingBytes = payload + (2 * pageSize);

#if TRIVIAL_PLATFORM_WINDOWS
	void* raw = VirtualAlloc(nullptr, mappingBytes, MEM_RESERVE, PAGE_NOACCESS);
	if (raw == nullptr) {
		outOsErrorCode = static_cast<int>(GetLastError());
		return nullptr;
	}

	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	void* payloadBase = static_cast<char*>(raw) + pageSize;

	if (VirtualAlloc(payloadBase, payload, MEM_COMMIT, PAGE_READWRITE) == nullptr) {
		outOsErrorCode = static_cast<int>(GetLastError());
		(void)VirtualFree(raw, 0, MEM_RELEASE);
		return nullptr;
	}

#elif TRIVIAL_PLATFORM_POSIX
	void* raw = mmap(nullptr, mappingBytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (raw == MAP_FAILED) {
		outOsErrorCode = errno;
		return nullptr;
	}

	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	void* payloadBase = static_cast<char*>(raw) + pageSize;

	if (mprotect(payloadBase, payload, PROT_READ | PROT_WRITE) != 0) {
		outOsErrorCode = errno;
		(void)munmap(raw, mappingBytes);
		return nullptr;
	}

#endif // Platform check

	outMappingBytes = mappingBytes;
	return payloadBase;
}

void unmapMetadata(void* payloadBase, std::size_t mappingBytes, std::size_t pageSize) noexcept {
	if (payloadBase == nullptr) {
		return;
	}

	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	void* raw = static_cast<char*>(payloadBase) - pageSize;

#if TRIVIAL_PLATFORM_WINDOWS
	(void)mappingBytes;

	if (VirtualFree(raw, 0, MEM_RELEASE) == 0) {
		TRIVIAL_LOG_ERROR_PREFIX("SegmentAllocator", "metadata release failed (VirtualFree)");
	}

#elif TRIVIAL_PLATFORM_POSIX
	if (munmap(raw, mappingBytes) != 0) {
		TRIVIAL_LOG_ERROR_PREFIX("SegmentAllocator", "metadata release failed (munmap)");
	}

#endif // Platform check
}

bool isBitSet(const std::uint64_t* bits, std::size_t index) noexcept {
	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	return (bits[index >> g_kWordShift] & (std::uint64_t{1} << (index & g_kBitIndexMask))) != 0;
}

void setBit(std::uint64_t* bits, std::size_t index) noexcept {
	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	bits[index >> g_kWordShift] |= std::uint64_t{1} << (index & g_kBitIndexMask);
}

void clearBit(std::uint64_t* bits, std::size_t index) noexcept {
	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	bits[index >> g_kWordShift] &= ~(std::uint64_t{1} << (index & g_kBitIndexMask));
}

// Can for sure SIMD this, but only worth bothering once the rutime dispatch is
// in place and there is a workload that requires enough RAM for this to become
// a bottleneck

std::size_t findCachedRun(const std::uint64_t* cached, std::size_t capacity, std::size_t count) noexcept {
	if (count == 0 || count > capacity) {
		return g_kInvalidIndex;
	}

	std::size_t run = 0;

	for (std::size_t index = 0; index < capacity; ++index) {
		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
		if ((index & g_kBitIndexMask) == 0 && cached[index >> g_kWordShift] == 0) {
			index += g_kBitIndexMask;
			run = 0;
			continue;
		}

		if (isBitSet(cached, index)) {
			++run;

			if (run == count) {
				return index + 1 - count;
			}
		} else {
			run = 0;
		}
	}

	return g_kInvalidIndex;
}

std::size_t findFreeRun(const std::uint64_t* allocated, std::size_t capacity, std::size_t count) noexcept {
	if (count == 0 || count > capacity) {
		return g_kInvalidIndex;
	}

	std::size_t run = 0;

	for (std::size_t index = 0; index < capacity; ++index) {
		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
		if ((index & g_kBitIndexMask) == 0 && allocated[index >> g_kWordShift] == g_kFullWord) {
			index += g_kBitIndexMask;
			run = 0;
			continue;
		}

		if (!isBitSet(allocated, index)) {
			++run;

			if (run == count) {
				return index + 1 - count;
			}
		} else {
			run = 0;
		}
	}

	return g_kInvalidIndex;
}

} // namespace

namespace trivial::memory {

[[nodiscard]] bool SegmentAllocator::init(std::size_t reserveBytes) noexcept {
	TRIVIAL_ASSERT(reserveBytes > 0);
	TRIVIAL_ASSERT(m_base == nullptr);

	bool needsOomReport = false;
	std::size_t oomRequestedSize = 0;
	const char* oomContext = nullptr;
	int oomErrorCode = 0;
	bool succeeded = false;

	{
		std::lock_guard lock(m_stateMutex);

		const SystemInfo kSystemInfo = probeSystemInfo();
		// NOTE: Could save this varible when page size is known but makes code
		//       even worse to read and would need adjustment of
		//       probeSystemInfo() based on debug mode
		const std::size_t kPageSize = kSystemInfo.pageSize;

#if TRIVIAL_PLATFORM_PAGE_SIZE_KNOWN
		TRIVIAL_ASSERT(kPageSize == core::g_kPageSize);
		TRIVIAL_ASSERT(kSystemInfo.allocationGranularity == core::g_kAllocationGranularity);
#else
		m_capabilities.pageSize = kPageSize;
		m_capabilities.allocationGranularity = kSystemInfo.allocationGranularity;
#endif // TRIVIAL_PLATFORM_PAGE_SIZE_KNOWN

#if TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
		m_capabilities.largePageSize = kSystemInfo.largePageSize;
#endif // TRIVIAL_MEMORY_ENABLE_LARGE_PAGES

#if TRIVIAL_MEMORY_ENABLE_DECOMMIT && TRIVIAL_PLATFORM_POSIX
		const bool kLazy = TRIVIAL_MEMORY_PREFER_LAZY_DECOMMIT && probeMadvFree(kPageSize);
		m_capabilities.decommitMode = kLazy ? DecommitMode::Lazy : DecommitMode::Eager;
#endif // TRIVIAL_MEMORY_ENABLE_DECOMMIT && TRIVIAL_PLATFORM_POSIX

		const std::size_t kTotalBytes = (reserveBytes + g_kSegmentMask) & ~g_kSegmentMask;

#if TRIVIAL_PLATFORM_WINDOWS
		void* rawBase = nullptr;
		std::size_t rawBytes = 0;
		void* const kBase = reserveAligned(kTotalBytes, g_kSegmentSize, kSystemInfo, rawBase, rawBytes, oomErrorCode);
#else
		void* const kBase = reserveAligned(kTotalBytes, g_kSegmentSize, kSystemInfo, oomErrorCode);
#endif // TRIVIAL_PLATFORM_WINDOWS

		if (kBase == nullptr) {
			needsOomReport = true;
			oomRequestedSize = reserveBytes;
			oomContext = "SegmentAllocator::init reservation failed";
		} else {
			m_base = kBase;
			m_segmentCapacity = kTotalBytes >> g_kSegmentShift;

#if TRIVIAL_PLATFORM_WINDOWS
			m_reservation = rawBase;
			m_reservationBytes = rawBytes;
#endif // TRIVIAL_PLATFORM_WINDOWS

			const std::size_t kBitmapWords = (m_segmentCapacity + g_kBitsPerWord - 1) >> g_kWordShift;
			const std::size_t kBitmapBytes = kBitmapWords * sizeof(std::uint64_t);
#if TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
			constexpr std::size_t kBitmapCount = 3; // Additionaly a pinned tracker
#else
			constexpr std::size_t kBitmapCount = 2; // Allocated and cached bitmaps
#endif // TRIVIAL_MEMORY_ENABLE_LARGE_PAGES

			const std::size_t kMetadataBytes
			    = (kBitmapCount * kBitmapBytes) + (m_segmentCapacity * sizeof(SegmentRecord));

			int metadataError = 0;
			void* const kMetadata = mapMetadata(kMetadataBytes, kPageSize, m_metadataMappingBytes, metadataError);

			if (kMetadata == nullptr) {
#if TRIVIAL_PLATFORM_WINDOWS
				releaseReservation(rawBase, rawBytes);

				m_reservation = nullptr;
				m_reservationBytes = 0;
#else
				releaseReservation(m_base, kTotalBytes);
#endif // TRIVIAL_PLATFORM_WINDOWS

				m_base = nullptr;
				m_segmentCapacity = 0;
				m_metadataMappingBytes = 0;

				needsOomReport = true;
				oomRequestedSize = kMetadataBytes;
				oomContext = "SegmentAllocator::init metadata mapping failed";
				oomErrorCode = metadataError;
			} else {
				m_metadata = kMetadata;

				char* cursor = static_cast<char*>(kMetadata);

				// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
				m_allocatedBitmap = reinterpret_cast<std::uint64_t*>(cursor);
				// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
				cursor += kBitmapBytes;

				// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
				m_cachedBitmap = reinterpret_cast<std::uint64_t*>(cursor);
				// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
				cursor += kBitmapBytes;

#if TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
				// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
				m_pinnedBitmap = reinterpret_cast<std::uint64_t*>(cursor);
				cursor += kBitmapBytes;
#endif // TRIVIAL_MEMORY_ENABLE_LARGE_PAGES

				// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
				m_records = reinterpret_cast<SegmentRecord*>(cursor);

				succeeded = true;
			}
		}
	}

	if (needsOomReport) {
		handleOom(oomRequestedSize, oomContext, oomErrorCode);
	}

	return succeeded;
}

void SegmentAllocator::shutdown() noexcept {
	std::lock_guard lock(m_stateMutex);

	if (m_base == nullptr) {
		return;
	}

#if TRIVIAL_PLATFORM_WINDOWS
	releaseReservation(m_reservation, m_reservationBytes);

	m_reservation = nullptr;
	m_reservationBytes = 0;
#else
	releaseReservation(m_base, m_segmentCapacity << g_kSegmentShift);
#endif // TRIVIAL_PLATFORM_WINDOWS

	unmapMetadata(m_metadata, m_metadataMappingBytes, trivial::memory::MemoryCapabilities::pageSize);

	m_metadata = nullptr;
	m_metadataMappingBytes = 0;

	m_base = nullptr;
	m_segmentCapacity = 0;
	m_allocatedBitmap = nullptr;
	m_cachedBitmap = nullptr;
#if TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
	m_pinnedBitmap = nullptr;
#endif // TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
	m_records = nullptr;
	m_highWaterSegments = 0;
	m_cachedSegments = 0;
#if TRIVIAL_MEMORY_ENABLE_DECOMMIT
	m_purgeCursor = 0;
#endif // TRIVIAL_MEMORY_ENABLE_DECOMMIT
#if TRIVIAL_MEMORY_ENABLE_TICK
	m_tick = 0;
#endif // TRIVIAL_MEMORY_ENABLE_TICK
}

[[nodiscard]] void* SegmentAllocator::allocSegments(std::size_t count, SegmentKind kind) noexcept {
	TRIVIAL_PROFILE_FUNCTION();
	TRIVIAL_ASSERT(count > 0);
	TRIVIAL_ASSERT(m_base != nullptr);

	bool needsOomReport = false;
	void* result = nullptr;

	{
		std::lock_guard lock(m_stateMutex);

		std::size_t index = g_kInvalidIndex;

		if (m_cachedSegments >= count) {
			index = findCachedRun(m_cachedBitmap, m_segmentCapacity, count);
		}

		if (index == g_kInvalidIndex) {
			index = findFreeRun(m_allocatedBitmap, m_segmentCapacity, count);
		}

		if (index == g_kInvalidIndex) {
			needsOomReport = true;
		} else {
			for (std::size_t offset = 0; offset < count; ++offset) {
				const std::size_t kSegment = index + offset;

				TRIVIAL_ASSERT(!isBitSet(m_allocatedBitmap, kSegment));

				if (isBitSet(m_cachedBitmap, kSegment)) {
					clearBit(m_cachedBitmap, kSegment);
					--m_cachedSegments;
				}

				setBit(m_allocatedBitmap, kSegment);

				// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
				m_records[kSegment].runLength = static_cast<std::uint16_t>(count);
#if TRIVIAL_ENABLE_MEMORY_DEBUG_STATS
				// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
				m_records[kSegment].kind = kind;
#endif // TRIVIAL_ENABLE_MEMORY_DEBUG_STATS
			}

			m_highWaterSegments = std::max(m_highWaterSegments, index + count);

			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
			result = static_cast<char*>(m_base) + (index << g_kSegmentShift);
		}
	}

	if (result != nullptr) {
		TRIVIAL_PROFILE_ALLOC("segments", result, count << g_kSegmentShift);
	}

#if !TRIVIAL_ENABLE_MEMORY_DEBUG_STATS
	(void)kind;
#endif // !TRIVIAL_ENABLE_MEMORY_DEBUG_STATS

	if (needsOomReport) {
		handleOom(count << g_kSegmentShift, "SegmentAllocator::allocSegments exhausted reservation", 0);
	}

	return result;
}

void SegmentAllocator::freeSegments(void* segments, std::size_t count) noexcept {
	TRIVIAL_PROFILE_FUNCTION();
	TRIVIAL_ASSERT(segments != nullptr);
	TRIVIAL_ASSERT(count > 0);
	TRIVIAL_ASSERT(owns(segments));

	TRIVIAL_PROFILE_FREE("segments", segments);

	std::lock_guard lock(m_stateMutex);

	const std::size_t kIndex = segmentIndex(segments);
	TRIVIAL_ASSERT(kIndex + count <= m_segmentCapacity);
	TRIVIAL_ASSERT(m_records[kIndex].runLength == count); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

	for (std::size_t offset = 0; offset < count; ++offset) {
		const std::size_t kSegment = kIndex + offset;

		TRIVIAL_ASSERT(isBitSet(m_allocatedBitmap, kSegment));
		TRIVIAL_ASSERT(!isBitSet(m_cachedBitmap, kSegment));

		clearBit(m_allocatedBitmap, kSegment);

		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
		SegmentRecord& record = m_records[kSegment];

		record = SegmentRecord{.committedPages = record.committedPages};

#if TRIVIAL_MEMORY_ENABLE_DECOMMIT
		if (m_cachedSegments >= g_kMaxCachedSegments) {
			purgeSegment(kSegment);
			continue;
		}
#endif // TRIVIAL_MEMORY_ENABLE_DECOMMIT

		if (record.committedPages > 0) {
#if TRIVIAL_MEMORY_ENABLE_DECOMMIT
			record.lastFreeTick = m_tick;
#endif // TRIVIAL_MEMORY_ENABLE_DECOMMIT
			setBit(m_cachedBitmap, kSegment);
			++m_cachedSegments;
		}
	}
}

[[nodiscard]] std::size_t SegmentAllocator::committedPages(const void* segment) const noexcept {
	if (!owns(segment)) {
		return 0;
	}

	std::lock_guard lock(m_stateMutex);
	return m_records[segmentIndex(segment)].committedPages; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
}

#if TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
[[nodiscard]] bool SegmentAllocator::enableLargePages() noexcept {
	if (m_capabilities.largePageSize == 0) {
		TRIVIAL_LOG_WARNING_PREFIX("SegmentAllocator", "large pages unsupported, falling back to normal pages");
		return false;
	}

#if TRIVIAL_PLATFORM_WINDOWS
	m_largePagesEnabled = adjustLockMemoryPrivilege(true);
#else
	m_largePagesEnabled = true;
#endif // TRIVIAL_PLATFORM_WINDOWS

	if (!m_largePagesEnabled) {
		TRIVIAL_LOG_WARNING_PREFIX("SegmentAllocator", "large pages denied, falling back to normal pages");
	}

	return m_largePagesEnabled;
}

void SegmentAllocator::disableLargePages() noexcept {
	if (!m_largePagesEnabled) {
		return;
	}

#if TRIVIAL_PLATFORM_WINDOWS
	(void)adjustLockMemoryPrivilege(false);
#endif // TRIVIAL_PLATFORM_WINDOWS

	m_largePagesEnabled = false;
}
#endif // TRIVIAL_MEMORY_ENABLE_LARGE_PAGES

[[nodiscard]] bool SegmentAllocator::ensureCommittedPages(void* segment,
                                                          std::size_t pages,
                                                          int& outOsErrorCode) noexcept {
	TRIVIAL_PROFILE_FUNCTION();
	TRIVIAL_ASSERT(segment != nullptr);
	TRIVIAL_ASSERT(owns(segment));
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	TRIVIAL_ASSERT((reinterpret_cast<std::uintptr_t>(segment) & g_kSegmentMask) == 0);

	const std::size_t kPageSize = m_capabilities.pageSize; // NOLINT(readability-static-accessed-through-instance)
	TRIVIAL_ASSERT(pages <= g_kSegmentSize / kPageSize);

	std::size_t bytes = 0;
	void* target = nullptr;

	{
		std::lock_guard lock(m_stateMutex);

		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
		SegmentRecord& record = m_records[segmentIndex(segment)];

		if (record.committedPages >= pages) {
			return true;
		}

		const std::size_t kAlready = record.committedPages;
		bytes = (pages - kAlready) * kPageSize;

		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
		target = static_cast<char*>(segment) + (kAlready * kPageSize);
	}

#if TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES
	if (!claimCommitBudget(bytes)) {
		outOsErrorCode = 0;
		handleOom(bytes, "SegmentAllocator::ensureCommittedPages exceeds commit budget", 0);
		return false;
	}
#endif // TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES

	if (!commitPages(target, bytes, kPageSize, outOsErrorCode)) {
#if TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES
		releaseCommitBudget(bytes);
#endif // TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES
		return false;
	}

	{
		std::lock_guard lock(m_stateMutex);

		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
		SegmentRecord& record = m_records[segmentIndex(segment)];

		if (record.committedPages < pages) {
			record.committedPages = static_cast<std::uint16_t>(pages);
		}
	}

	return true;
}

#if TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
[[nodiscard]] bool SegmentAllocator::ensureCommittedLargePages(void* segment,
                                                               std::size_t pages,
                                                               int& outOsErrorCode) noexcept {
	TRIVIAL_PROFILE_FUNCTION();
	TRIVIAL_ASSERT(segment != nullptr);
	TRIVIAL_ASSERT(owns(segment));
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	TRIVIAL_ASSERT((reinterpret_cast<std::uintptr_t>(segment) & g_kSegmentMask) == 0);

	if (!m_largePagesEnabled) {
		return ensureCommittedPages(segment, pages, outOsErrorCode);
	}

	const std::size_t kPageSize = m_capabilities.pageSize;
	TRIVIAL_ASSERT(pages <= g_kSegmentSize / kPageSize);
	TRIVIAL_ASSERT((pages * kPageSize) % m_capabilities.largePageSize == 0);

	std::size_t bytes = 0;
	void* target = nullptr;

	{
		std::lock_guard lock(m_stateMutex);

		SegmentRecord& record = m_records[segmentIndex(segment)];
		if (record.committedPages >= pages) {
			return true;
		}

		const std::size_t kAlready = record.committedPages;
		bytes = (pages - kAlready) * kPageSize;

		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
		target = static_cast<char*>(segment) + (kAlready * kPageSize);
	}

#if TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES
	if (!claimCommitBudget(bytes)) {
		outOsErrorCode = 0;
		handleOom(bytes, "SegmentAllocator::ensureCommittedLargePages exceeds commit budget", 0);
		return false;
	}
#endif // TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES

	if (!commitLargePages(target, bytes, m_capabilities.largePageSize, outOsErrorCode)) {
#if TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES
		releaseCommitBudget(bytes);
#endif // TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES
		return false;
	}

	{
		std::lock_guard lock(m_stateMutex);

		const std::size_t kIndex = segmentIndex(segment);
		SegmentRecord& record = m_records[kIndex];

		if (record.committedPages < pages) {
			record.committedPages = static_cast<std::uint16_t>(pages);
		}

#if TRIVIAL_PLATFORM_WINDOWS
		// Large page backing is locked and cannot be partially released, so the
		// segment is withheld from the purge sweep.
		setBit(m_pinnedBitmap, kIndex);
#endif // TRIVIAL_PLATFORM_WINDOWS
	}

	return true;
}
#endif // TRIVIAL_MEMORY_ENABLE_LARGE_PAGES

void SegmentAllocator::trimCommittedPagesTo(void* segment, std::size_t pages) noexcept {
	TRIVIAL_PROFILE_FUNCTION();
	TRIVIAL_ASSERT(segment != nullptr);
	TRIVIAL_ASSERT(owns(segment));
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	TRIVIAL_ASSERT((reinterpret_cast<std::uintptr_t>(segment) & g_kSegmentMask) == 0);

	const std::size_t kPageSize = m_capabilities.pageSize; // NOLINT(readability-static-accessed-through-instance)

	std::lock_guard lock(m_stateMutex);

	const std::size_t kIndex = segmentIndex(segment);

#if TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
	if (isBitSet(m_pinnedBitmap, kIndex)) {
		return;
	}
#endif // TRIVIAL_MEMORY_ENABLE_LARGE_PAGES

	SegmentRecord& record = m_records[kIndex]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	if (record.committedPages <= pages) {
		return;
	}

	const std::size_t kBytes = (record.committedPages - pages) * kPageSize;

	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	void* target = static_cast<char*>(segment) + (pages * kPageSize);

	decommitRange(target, kBytes);
	record.committedPages = static_cast<std::uint16_t>(pages);
}

void SegmentAllocator::decommitRange(void* addr, std::size_t bytes) const noexcept {
	// NOLINTNEXTLINE(readability-static-accessed-through-instance)
	decommitPages(addr, bytes, m_capabilities.pageSize, m_capabilities.decommitMode);

#if TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES
	releaseCommitBudget(bytes);
#endif // TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES
}

#if TRIVIAL_MEMORY_ENABLE_TICK
void SegmentAllocator::tick() noexcept {
	TRIVIAL_PROFILE_FUNCTION();

	std::lock_guard lock(m_stateMutex);

	++m_tick;

#if TRIVIAL_ENABLE_MEMORY_DEBUG_STATS
	TRIVIAL_PROFILE_VALUE("memory/committed", static_cast<std::int64_t>(committedBytes()));
	TRIVIAL_PROFILE_VALUE("memory/cachedSegments", static_cast<std::int64_t>(m_cachedSegments));
	TRIVIAL_PROFILE_VALUE("memory/highWaterSegments", static_cast<std::int64_t>(m_highWaterSegments));
#endif // TRIVIAL_ENABLE_MEMORY_DEBUG_STATS

#if TRIVIAL_MEMORY_ENABLE_DECOMMIT
	if (m_cachedSegments == 0) {
		return;
	}

	const std::size_t kBudget = std::clamp(m_cachedSegments / g_kPurgeFraction, g_kMinPurgePerTick, g_kMaxPurgePerTick);

	std::size_t scanned = 0;
	std::size_t purged = 0;

	while (scanned < m_segmentCapacity && purged < kBudget) {
		const std::size_t kSegment = m_purgeCursor;

		m_purgeCursor = m_purgeCursor + 1 < m_segmentCapacity ? m_purgeCursor + 1 : 0;
		++scanned;

		if (!isBitSet(m_cachedBitmap, kSegment)) {
			continue;
		}

#if TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
		if (isBitSet(m_pinnedBitmap, kSegment)) {
			continue;
		}
#endif // TRIVIAL_MEMORY_ENABLE_LARGE_PAGES

		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
		const std::uint32_t kAge = static_cast<std::uint32_t>(m_tick - m_records[kSegment].lastFreeTick);

		if (kAge < g_kDecayTicks) {
			continue;
		}

		purgeSegment(kSegment);
		++purged;
	}
#endif // TRIVIAL_MEMORY_ENABLE_DECOMMIT
}
#endif // TRIVIAL_MEMORY_ENABLE_TICK

void SegmentAllocator::purgeSegment(std::size_t segment) noexcept {
	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	SegmentRecord& record = m_records[segment];

	if (record.committedPages > 0) {
		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
		void* addr = static_cast<char*>(m_base) + (segment << g_kSegmentShift);
		// NOLINTNEXTLINE(readability-static-accessed-through-instance)
		const std::size_t kBytes = static_cast<std::size_t>(record.committedPages) * m_capabilities.pageSize;

		decommitRange(addr, kBytes);
	}

	record = SegmentRecord{};

#if TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
	clearBit(m_pinnedBitmap, segment);
#endif // TRIVIAL_MEMORY_ENABLE_LARGE_PAGES

	if (isBitSet(m_cachedBitmap, segment)) {
		clearBit(m_cachedBitmap, segment);
		--m_cachedSegments;
	}
}

#if TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES
[[nodiscard]] bool SegmentAllocator::claimCommitBudget(std::size_t bytes) const noexcept {
#if TRIVIAL_MEMORY_ENABLE_COMMIT_BUDGET
#if TRIVIAL_MEMORY_FIXED_COMMIT_BUDGET
	constexpr std::size_t kLimit = g_kCommitBudgetBytes;
#else
	const std::size_t kLimit = m_commitBudgetBytes;
#endif // TRIVIAL_MEMORY_FIXED_COMMIT_BUDGET

	if (kLimit != 0) {
		std::size_t current = m_committedBytes.load(std::memory_order_relaxed);

		while (true) {
			if (current + bytes > kLimit) {
				return false;
			}

			if (m_committedBytes.compare_exchange_weak(current, current + bytes, std::memory_order_relaxed)) {
				return true;
			}
		}
	}
#endif // TRIVIAL_MEMORY_ENABLE_COMMIT_BUDGET

	m_committedBytes.fetch_add(bytes, std::memory_order_relaxed);
	return true;
}

void SegmentAllocator::releaseCommitBudget(std::size_t bytes) const noexcept {
	TRIVIAL_ASSERT(m_committedBytes.load(std::memory_order_relaxed) >= bytes);
	m_committedBytes.fetch_sub(bytes, std::memory_order_relaxed);
}
#endif // TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES

[[nodiscard]] bool SegmentAllocator::owns(const void* ptr) const noexcept {
	if (m_base == nullptr || ptr == nullptr) {
		return false;
	}

	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	const auto kAddress = reinterpret_cast<std::uintptr_t>(ptr);
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	const auto kBase = reinterpret_cast<std::uintptr_t>(m_base);

	return kAddress >= kBase && kAddress < kBase + (m_segmentCapacity << g_kSegmentShift);
}

[[nodiscard]] std::size_t SegmentAllocator::segmentIndex(const void* ptr) const noexcept {
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	const auto kAddress = reinterpret_cast<std::uintptr_t>(ptr);
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	const auto kBase = reinterpret_cast<std::uintptr_t>(m_base);

	return static_cast<std::size_t>(kAddress - kBase) >> g_kSegmentShift;
}

#if TRIVIAL_ENABLE_MEMORY_DEBUG_STATS
[[nodiscard]] SegmentKind SegmentAllocator::kindOf(const void* ptr) const noexcept {
	if (!owns(ptr)) {
		return SegmentKind::Invalid;
	}

	std::lock_guard lock(m_stateMutex);

	std::size_t index = segmentIndex(ptr);
	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	return isBitSet(m_allocatedBitmap, index) ? m_records[index].kind : SegmentKind::Invalid;
}

[[nodiscard]] SegmentRecord SegmentAllocator::recordOf(const void* ptr) const noexcept {
	if (!owns(ptr)) {
		return SegmentRecord{};
	}

	std::lock_guard lock(m_stateMutex);
	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	return m_records[segmentIndex(ptr)];
}
#endif // TRIVIAL_ENABLE_MEMORY_DEBUG_STATS

void SegmentAllocator::handleOom(std::size_t requestedSize, const char* context, int osErrorCode) const noexcept {
	TRIVIAL_LOG_OOM_FAILURE("SegmentAllocator", context, requestedSize, osErrorCode);

	std::lock_guard lock(m_oomMutex);

	if (m_oomHandler != nullptr) {
		OomInfo info{.requestedSize = requestedSize, .context = context, .osErrorCode = osErrorCode};
		m_oomHandler(info);
	}
}

} // namespace trivial::memory

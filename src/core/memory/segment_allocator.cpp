#include "core/memory/segment_allocator.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>

#include <trivial/core/assert.h>
#include <trivial/core/log.h>
#include <trivial/core/memory/memory_config.h>
#include <trivial/core/memory/oom_handler.h>
#include <trivial/core/platform.h>
#include <trivial/core/profile.h>
#include <trivial/core/sync/lock_guard.h>

#include "core/memory/memory_capabilities.h"
#include "core/memory/virtual_memory.h"

#if TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES
#include <atomic>
#endif // TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES

#if TRIVIAL_PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h> // IWYU pragma: keep

#elif TRIVIAL_PLATFORM_POSIX
#include <cerrno>
#include <sys/mman.h>

#else
#error "Unsupported platform in segment_allocator.cpp"

#endif // Platform check

// TODO: Some of these functions would for sure benefit from the safety gained
//       from using a named struct

#define TRIVIAL_MEMORY_SEGMENT_ALLOCATOR_INVALID_INDEX (std::numeric_limits<std::size_t>::max())

#define TRIVIAL_MEMORY_SEGMENT_ALLOCATOR_BITS_PER_WORD                                                                 \
	(static_cast<std::size_t>(std::numeric_limits<std::uint64_t>::digits))
#define TRIVIAL_MEMORY_SEGMENT_ALLOCATOR_WORD_SHIFT                                                                    \
	(static_cast<std::size_t>(std::countr_zero(TRIVIAL_MEMORY_SEGMENT_ALLOCATOR_BITS_PER_WORD)))
#define TRIVIAL_MEMORY_SEGMENT_ALLOCATOR_BIT_INDEX_MASK                                                                \
	(TRIVIAL_MEMORY_SEGMENT_ALLOCATOR_BITS_PER_WORD - std::size_t{1})
#define TRIVIAL_MEMORY_SEGMENT_ALLOCATOR_FULL_WORD (~std::uint64_t{0})

namespace {

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

#if TRIVIAL_PLATFORM_WINDOWS
	void* raw = VirtualAlloc(nullptr, kMappingBytes, MEM_RESERVE, PAGE_NOACCESS);
	if (raw == nullptr) {
		outOsErrorCode = static_cast<int>(GetLastError());
		return nullptr;
	}

	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	void* payloadBase = static_cast<char*>(raw) + pageSize;

	if (VirtualAlloc(payloadBase, kPayload, MEM_COMMIT, PAGE_READWRITE) == nullptr) {
		outOsErrorCode = static_cast<int>(GetLastError());
		(void)VirtualFree(raw, 0, MEM_RELEASE);
		return nullptr;
	}

#elif TRIVIAL_PLATFORM_POSIX
	void* raw = mmap(nullptr, kMappingBytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (raw == MAP_FAILED) {
		outOsErrorCode = errno;
		return nullptr;
	}

	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	void* payloadBase = static_cast<char*>(raw) + pageSize;

	if (mprotect(payloadBase, kPayload, PROT_READ | PROT_WRITE) != 0) {
		outOsErrorCode = errno;
		(void)munmap(raw, kMappingBytes);
		return nullptr;
	}

#endif // Platform check

	outMappingBytes = kMappingBytes;
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

[[nodiscard]] constexpr bool isBitSet(const std::uint64_t* bits, std::size_t index) noexcept {
	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	return (bits[index >> TRIVIAL_MEMORY_SEGMENT_ALLOCATOR_WORD_SHIFT]
	        & (std::uint64_t{1} << (index & TRIVIAL_MEMORY_SEGMENT_ALLOCATOR_BIT_INDEX_MASK)))
	       != 0;
}

constexpr void setBit(std::uint64_t* bits, std::size_t index) noexcept {
	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	bits[index >> TRIVIAL_MEMORY_SEGMENT_ALLOCATOR_WORD_SHIFT]
	    |= std::uint64_t{1} << (index & TRIVIAL_MEMORY_SEGMENT_ALLOCATOR_BIT_INDEX_MASK);
}

constexpr void clearBit(std::uint64_t* bits, std::size_t index) noexcept {
	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	bits[index >> TRIVIAL_MEMORY_SEGMENT_ALLOCATOR_WORD_SHIFT]
	    &= ~(std::uint64_t{1} << (index & TRIVIAL_MEMORY_SEGMENT_ALLOCATOR_BIT_INDEX_MASK));
}

// NOTE: Can for sure SIMD this, but only worth bothering once the rutime
// dispatch is in place and there is a workload that requires enough RAM for
// this to become a bottleneck
[[nodiscard]] constexpr std::size_t findCachedRun(const std::uint64_t* cached,
                                                  std::size_t capacity,
                                                  std::size_t count) noexcept {
	if (count == 0 || count > capacity) {
		return TRIVIAL_MEMORY_SEGMENT_ALLOCATOR_INVALID_INDEX;
	}

	std::size_t run = 0;

	for (std::size_t index = 0; index < capacity; ++index) {
		const bool kWordStart = (index & TRIVIAL_MEMORY_SEGMENT_ALLOCATOR_BIT_INDEX_MASK) == 0;
		const std::size_t kWordIndex = index >> TRIVIAL_MEMORY_SEGMENT_ALLOCATOR_WORD_SHIFT;

		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
		if (kWordStart && cached[kWordIndex] == 0) {
			index += TRIVIAL_MEMORY_SEGMENT_ALLOCATOR_BIT_INDEX_MASK;
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

	return TRIVIAL_MEMORY_SEGMENT_ALLOCATOR_INVALID_INDEX;
}

[[nodiscard]] constexpr std::size_t findFreeRun(const std::uint64_t* allocated,
                                                std::size_t capacity,
                                                std::size_t count) noexcept {
	if (count == 0 || count > capacity) {
		return TRIVIAL_MEMORY_SEGMENT_ALLOCATOR_INVALID_INDEX;
	}

	std::size_t run = 0;

	for (std::size_t index = 0; index < capacity; ++index) {
		const bool kWordStart = (index & TRIVIAL_MEMORY_SEGMENT_ALLOCATOR_BIT_INDEX_MASK) == 0;
		const std::size_t kWordIndex = index >> TRIVIAL_MEMORY_SEGMENT_ALLOCATOR_WORD_SHIFT;

		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
		if (kWordStart && allocated[kWordIndex] == TRIVIAL_MEMORY_SEGMENT_ALLOCATOR_FULL_WORD) {
			index += TRIVIAL_MEMORY_SEGMENT_ALLOCATOR_BIT_INDEX_MASK;
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

	return TRIVIAL_MEMORY_SEGMENT_ALLOCATOR_INVALID_INDEX;
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
		const sync::LockGuard kLock(m_stateMutex);

		const std::size_t kPageSize = pageSize();

		const std::size_t kTotalBytes = (reserveBytes + TRIVIAL_MEMORY_SEGMENT_MASK) & ~TRIVIAL_MEMORY_SEGMENT_MASK;

#if TRIVIAL_PLATFORM_WINDOWS
		void* rawBase = nullptr;
		std::size_t rawBytes = 0;
		void* const kBase = reserveAligned(kTotalBytes, TRIVIAL_MEMORY_SEGMENT_SIZE, rawBase, rawBytes, oomErrorCode);
#else
		void* const kBase = reserveAligned(kTotalBytes, TRIVIAL_MEMORY_SEGMENT_SIZE, oomErrorCode);
#endif // TRIVIAL_PLATFORM_WINDOWS

		if (kBase == nullptr) {
			needsOomReport = true;
			oomRequestedSize = reserveBytes;
			oomContext = "SegmentAllocator::init reservation failed";
		} else {
			m_base = kBase;
			m_segmentCapacity = kTotalBytes >> TRIVIAL_MEMORY_SEGMENT_SHIFT;

#if TRIVIAL_PLATFORM_WINDOWS
			m_reservation = rawBase;
			m_reservationBytes = rawBytes;
#endif // TRIVIAL_PLATFORM_WINDOWS

			const std::size_t kBitmapWords = (m_segmentCapacity + TRIVIAL_MEMORY_SEGMENT_ALLOCATOR_BITS_PER_WORD - 1)
			                                 >> TRIVIAL_MEMORY_SEGMENT_ALLOCATOR_WORD_SHIFT;
			const std::size_t kBitmapBytes = kBitmapWords * sizeof(std::uint64_t);
			constexpr std::size_t kBitmapCount = TRIVIAL_MEMORY_ENABLE_LARGE_PAGES ? 3 : 2;

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

	if (needsOomReport) [[unlikely]] {
		handleOom(oomRequestedSize, oomContext, oomErrorCode);
	}

	return succeeded;
}

void SegmentAllocator::shutdown() noexcept {
	const sync::LockGuard kLock(m_stateMutex);

	if (m_base == nullptr) {
		return;
	}

#if TRIVIAL_PLATFORM_WINDOWS
	releaseReservation(m_reservation, m_reservationBytes);

	m_reservation = nullptr;
	m_reservationBytes = 0;
#else
	releaseReservation(m_base, m_segmentCapacity << TRIVIAL_MEMORY_SEGMENT_SHIFT);
#endif // TRIVIAL_PLATFORM_WINDOWS

	unmapMetadata(m_metadata, m_metadataMappingBytes, pageSize());

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
	TRIVIAL_ASSUME(count > 0);
	TRIVIAL_ASSERT(m_base != nullptr);

	bool needsOomReport = false;
	void* result = nullptr;

	{
		const sync::LockGuard kLock(m_stateMutex);

		std::size_t index = TRIVIAL_MEMORY_SEGMENT_ALLOCATOR_INVALID_INDEX;

		if (m_cachedSegments >= count) {
			index = findCachedRun(m_cachedBitmap, m_segmentCapacity, count);
		}

		if (index == TRIVIAL_MEMORY_SEGMENT_ALLOCATOR_INVALID_INDEX) {
			index = findFreeRun(m_allocatedBitmap, m_segmentCapacity, count);
		}

		if (index == TRIVIAL_MEMORY_SEGMENT_ALLOCATOR_INVALID_INDEX) {
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
			result = static_cast<char*>(m_base) + (index << TRIVIAL_MEMORY_SEGMENT_SHIFT);
		}
	}

	if (result != nullptr) {
		TRIVIAL_PROFILE_ALLOC("segments", result, count << TRIVIAL_MEMORY_SEGMENT_SHIFT);
	}

	(void)kind;

	if (needsOomReport) [[unlikely]] {
		handleOom(count << TRIVIAL_MEMORY_SEGMENT_SHIFT, "SegmentAllocator::allocSegments exhausted reservation", 0);
	}

	return result;
}

void SegmentAllocator::freeSegments(void* segments, std::size_t count) noexcept {
	TRIVIAL_PROFILE_FUNCTION();
	TRIVIAL_ASSERT(segments != nullptr);
	TRIVIAL_ASSUME(count > 0);
	TRIVIAL_ASSERT(owns(segments));

	TRIVIAL_PROFILE_FREE("segments", segments);

	const sync::LockGuard kLock(m_stateMutex);

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
		if (m_cachedSegments >= TRIVIAL_MEMORY_MAX_CACHED_SEGMENTS) {
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

	const sync::LockGuard kLock(m_stateMutex);
	return m_records[segmentIndex(segment)].committedPages; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
}

#if TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
[[nodiscard]] bool SegmentAllocator::enableLargePages() noexcept {
	if (largePageSize() == 0) {
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

[[nodiscard]] bool SegmentAllocator::ensureCommittedLargePages(void* segment,
                                                               std::size_t pages,
                                                               int& outOsErrorCode) noexcept {
	TRIVIAL_PROFILE_FUNCTION();
	TRIVIAL_ASSERT(segment != nullptr);
	TRIVIAL_ASSERT(owns(segment));
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	TRIVIAL_ASSERT((reinterpret_cast<std::uintptr_t>(segment) & TRIVIAL_MEMORY_SEGMENT_MASK) == 0);

	if (!m_largePagesEnabled) {
		return ensureCommittedPages(segment, pages, outOsErrorCode);
	}

	const std::size_t kPageSize = pageSize();
	TRIVIAL_ASSERT(pages <= TRIVIAL_MEMORY_SEGMENT_SIZE / kPageSize);
	TRIVIAL_ASSERT((pages * kPageSize) % largePageSize() == 0);

	std::size_t bytes = 0;
	void* target = nullptr;

	{
		const sync::LockGuard kLock(m_stateMutex);

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
	if (!claimCommitBudget(bytes)) [[unlikely]] {
		outOsErrorCode = 0;
		handleOom(bytes, "SegmentAllocator::ensureCommittedLargePages exceeds commit budget", 0);
		return false;
	}
#endif // TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES

	if (!commitLargePages(target, bytes, largePageSize(), outOsErrorCode)) [[unlikely]] {
#if TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES
		releaseCommitBudget(bytes);
#endif // TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES
		return false;
	}

	{
		const sync::LockGuard kLock(m_stateMutex);

		const std::size_t kIndex = segmentIndex(segment);
		SegmentRecord& record = m_records[kIndex];

		if (record.committedPages < pages) {
			record.committedPages = static_cast<std::uint16_t>(pages);
		}

#if TRIVIAL_PLATFORM_WINDOWS
		// Large page backing is locked and cannot be partially released, so the
		// segment is withheld from the purge sweep
		setBit(m_pinnedBitmap, kIndex);
#endif // TRIVIAL_PLATFORM_WINDOWS
	}

	return true;
}
#endif // TRIVIAL_MEMORY_ENABLE_LARGE_PAGES

[[nodiscard]] bool SegmentAllocator::ensureCommittedPages(void* segment,
                                                          std::size_t pages,
                                                          int& outOsErrorCode) noexcept {
	TRIVIAL_PROFILE_FUNCTION();
	TRIVIAL_ASSERT(segment != nullptr);
	TRIVIAL_ASSERT(owns(segment));
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	TRIVIAL_ASSERT((reinterpret_cast<std::uintptr_t>(segment) & TRIVIAL_MEMORY_SEGMENT_MASK) == 0);

	const std::size_t kPageSize = pageSize();
	TRIVIAL_ASSERT(pages <= TRIVIAL_MEMORY_SEGMENT_SIZE / kPageSize);

	std::size_t bytes = 0;
	void* target = nullptr;

	{
		const sync::LockGuard kLock(m_stateMutex);

		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
		const SegmentRecord& record = m_records[segmentIndex(segment)];

		if (record.committedPages >= pages) {
			return true;
		}

		const std::size_t kAlready = record.committedPages;
		bytes = (pages - kAlready) * kPageSize;

		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
		target = static_cast<char*>(segment) + (kAlready * kPageSize);
	}

#if TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES
	if (!claimCommitBudget(bytes)) [[unlikely]] {
		outOsErrorCode = 0;
		handleOom(bytes, "SegmentAllocator::ensureCommittedPages exceeds commit budget", 0);
		return false;
	}
#endif // TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES

	if (!commitPages(target, bytes, kPageSize, outOsErrorCode)) [[unlikely]] {
#if TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES
		releaseCommitBudget(bytes);
#endif // TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES
		return false;
	}

	{
		const sync::LockGuard kLock(m_stateMutex);

		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
		SegmentRecord& record = m_records[segmentIndex(segment)];

		if (record.committedPages < pages) {
			record.committedPages = static_cast<std::uint16_t>(pages);
		}
	}

	return true;
}

#if TRIVIAL_MEMORY_ENABLE_DECOMMIT
void SegmentAllocator::trimCommittedPagesTo(void* segment, std::size_t pages) noexcept {
	TRIVIAL_PROFILE_FUNCTION();
	TRIVIAL_ASSERT(segment != nullptr);
	TRIVIAL_ASSERT(owns(segment));
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	TRIVIAL_ASSERT((reinterpret_cast<std::uintptr_t>(segment) & TRIVIAL_MEMORY_SEGMENT_MASK) == 0);

	const std::size_t kPageSize = pageSize();

	const sync::LockGuard kLock(m_stateMutex);

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

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void SegmentAllocator::decommitRange(void* addr, std::size_t bytes) const noexcept {
	decommitPages(addr, bytes, pageSize());

#if TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES
	releaseCommitBudget(bytes);
#endif // TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES
}
#endif // TRIVIAL_MEMORY_ENABLE_DECOMMIT

#if TRIVIAL_MEMORY_ENABLE_TICK
void SegmentAllocator::tick() noexcept {
	TRIVIAL_PROFILE_FUNCTION();

	const sync::LockGuard kLock(m_stateMutex);

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

	const std::size_t kBudget = std::clamp<std::size_t>(m_cachedSegments / TRIVIAL_MEMORY_PURGE_FRACTION,
	                                                    TRIVIAL_MEMORY_MIN_PURGE_PER_TICK,
	                                                    TRIVIAL_MEMORY_MAX_PURGE_PER_TICK);

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

		if (kAge < TRIVIAL_MEMORY_DECAY_TICKS) {
			continue;
		}

		purgeSegment(kSegment);
		++purged;
	}
#endif // TRIVIAL_MEMORY_ENABLE_DECOMMIT
}
#endif // TRIVIAL_MEMORY_ENABLE_TICK

#if TRIVIAL_MEMORY_ENABLE_DECOMMIT
void SegmentAllocator::purgeSegment(std::size_t segment) noexcept {
	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	SegmentRecord& record = m_records[segment];

	if (record.committedPages > 0) {
		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
		void* addr = static_cast<char*>(m_base) + (segment << TRIVIAL_MEMORY_SEGMENT_SHIFT);
		const std::size_t kBytes = static_cast<std::size_t>(record.committedPages) * pageSize();

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
#endif // TRIVIAL_MEMORY_ENABLE_DECOMMIT

#if TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES
[[nodiscard]] bool SegmentAllocator::claimCommitBudget(std::size_t bytes) const noexcept {
#if TRIVIAL_MEMORY_ENABLE_COMMIT_BUDGET
#if TRIVIAL_MEMORY_FIXED_COMMIT_BUDGET
	constexpr std::size_t kLimit = TRIVIAL_MEMORY_COMMIT_BUDGET_BYTES;
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
	const std::uintptr_t kAddress = reinterpret_cast<std::uintptr_t>(ptr);
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	const std::uintptr_t kBase = reinterpret_cast<std::uintptr_t>(m_base);

	return kAddress >= kBase && kAddress < kBase + (m_segmentCapacity << TRIVIAL_MEMORY_SEGMENT_SHIFT);
}

[[nodiscard]] std::size_t SegmentAllocator::segmentIndex(const void* ptr) const noexcept {
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	const std::uintptr_t kAddress = reinterpret_cast<std::uintptr_t>(ptr);
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	const std::uintptr_t kBase = reinterpret_cast<std::uintptr_t>(m_base);

	return static_cast<std::size_t>(kAddress - kBase) >> TRIVIAL_MEMORY_SEGMENT_SHIFT;
}

#if TRIVIAL_ENABLE_MEMORY_DEBUG_STATS
[[nodiscard]] SegmentKind SegmentAllocator::kindOf(const void* ptr) const noexcept {
	if (!owns(ptr)) {
		return SegmentKind::Invalid;
	}

	const sync::LockGuard kLock(m_stateMutex);

	const std::size_t kIndex = segmentIndex(ptr);
	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	return isBitSet(m_allocatedBitmap, kIndex) ? m_records[kIndex].kind : SegmentKind::Invalid;
}

[[nodiscard]] SegmentRecord SegmentAllocator::recordOf(const void* ptr) const noexcept {
	if (!owns(ptr)) {
		return SegmentRecord{};
	}

	const sync::LockGuard kLock(m_stateMutex);
	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	return m_records[segmentIndex(ptr)];
}
#endif // TRIVIAL_ENABLE_MEMORY_DEBUG_STATS

void SegmentAllocator::handleOom(std::size_t requestedSize, const char* context, int osErrorCode) const noexcept {
	TRIVIAL_LOG_OOM_FAILURE("SegmentAllocator", context, requestedSize, osErrorCode);

	const sync::LockGuard kLock(m_oomMutex);

	if (m_oomHandler != nullptr) {
		const OomInfo kInfo{.requestedSize = requestedSize, .context = context, .osErrorCode = osErrorCode};
		m_oomHandler(kInfo);
	}
}

} // namespace trivial::memory

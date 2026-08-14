#ifndef TRIVIAL_CORE_MEMORY_SEGMENT_ALLOCATOR_H
#define TRIVIAL_CORE_MEMORY_SEGMENT_ALLOCATOR_H

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>

#include <trivial/core/config.h>
#include <trivial/core/memory/memory_config.h>
#include <trivial/core/memory/oom_handler.h>
#include <trivial/core/platform.h>

namespace trivial::memory {

inline constexpr std::uint8_t g_kLastGeneralKind = 15;

// NOTE: Upto g_kLastGeneralKind, which should remain 15, are reserved for the general allocator
enum class SegmentKind : std::uint8_t {
	Invalid = 0,

	Small = 1,
	Medium = 2,
	LargeSpan = 3,
	HugeHead = 4,
	HugeFollower = 5,

	FrameArena = 16,
	BufferedArena = 17,
	ScratchArena = 18,
	EcsChunks = 19,
	TaskSystem = 20,
	TexturePool = 21,

	External = 64,
};

enum class DecommitMode : std::uint8_t {
	Disabled,
	Eager,
	Lazy,
};

[[nodiscard]] constexpr bool isGeneralAllocatorKind(SegmentKind kind) noexcept {
	const std::uint8_t kValue = static_cast<std::uint8_t>(kind);
	return kValue != 0 && kValue <= g_kLastGeneralKind;
}

[[nodiscard]] constexpr const char* segmentKindName(SegmentKind kind) noexcept {
	switch (kind) {
		case SegmentKind::Invalid:
			return "invalid";
		case SegmentKind::Small:
			return "small";
		case SegmentKind::Medium:
			return "medium";
		case SegmentKind::LargeSpan:
			return "largeSpan";
		case SegmentKind::HugeHead:
			return "hugeHead";
		case SegmentKind::HugeFollower:
			return "hugeFollower";
		case SegmentKind::FrameArena:
			return "frameArena";
		case SegmentKind::BufferedArena:
			return "bufferedArena";
		case SegmentKind::ScratchArena:
			return "scratchArena";
		case SegmentKind::EcsChunks:
			return "ecsChunks";
		case SegmentKind::TaskSystem:
			return "taskSystem";
		case SegmentKind::TexturePool:
			return "texturePool";
		case SegmentKind::External:
			return "external";
		default:
			return "unknown";
	}
}

struct MemoryCapabilities {
#if TRIVIAL_PLATFORM_PAGE_SIZE_KNOWN
	// NOLINTNEXTLINE(readability-identifier-naming)
	static constexpr std::size_t pageSize = core::g_kPageSize;
	// NOLINTNEXTLINE(readability-identifier-naming)
	static constexpr std::size_t allocationGranularity = core::g_kAllocationGranularity;
#else
	std::size_t pageSize = 0;
	std::size_t allocationGranularity = 0;
#endif // TRIVIAL_PLATFORM_PAGE_SIZE_KNOWN

#if TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
	std::size_t largePageSize = 0;
#endif // TRIVIAL_MEMORY_ENABLE_LARGE_PAGES

#if !TRIVIAL_MEMORY_ENABLE_DECOMMIT
	static constexpr DecommitMode decommitMode = DecommitMode::Disabled;
#elif TRIVIAL_PLATFORM_WINDOWS
	// VirtualFree has no lazy equivalent, so the mode is never in question
	static constexpr DecommitMode decommitMode = DecommitMode::Eager;
#else
	// Eager or lazy depending on what the MADV_FREE probe found
	DecommitMode decommitMode = DecommitMode::Eager;
#endif // Decommit mode
};

struct SegmentRecord {
#if TRIVIAL_MEMORY_ENABLE_DECOMMIT
	std::uint32_t lastFreeTick = 0;
#endif // TRIVIAL_MEMORY_ENABLE_DECOMMIT
	std::uint16_t committedPages = 0;
	std::uint16_t runLength = 0;
#if TRIVIAL_ENABLE_MEMORY_DEBUG_STATS
	SegmentKind kind = SegmentKind::Invalid;
#endif // TRIVIAL_ENABLE_MEMORY_DEBUG_STATS
};

// init, shutdown, and the setters are not thread safe, tick may behave weirdly
// too, so just keep to calling from one threaed every so often
class SegmentAllocator {
public:
	SegmentAllocator() noexcept = default;

	~SegmentAllocator() { shutdown(); }

	SegmentAllocator(const SegmentAllocator&) = delete;
	SegmentAllocator& operator=(const SegmentAllocator&) = delete;

	SegmentAllocator(SegmentAllocator&&) = delete;
	SegmentAllocator& operator=(SegmentAllocator&&) = delete;

	[[nodiscard]] bool init(std::size_t reserveBytes) noexcept;

#if TRIVIAL_MEMORY_ENABLE_COMMIT_BUDGET && !TRIVIAL_MEMORY_FIXED_COMMIT_BUDGET
	void setCommitBudget(std::size_t bytes) noexcept { m_commitBudgetBytes = bytes; }
#endif // TRIVIAL_MEMORY_ENABLE_COMMIT_BUDGET && !TRIVIAL_MEMORY_FIXED_COMMIT_BUDGET

#if TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES
	[[nodiscard]] std::size_t committedBytes() const noexcept {
		return m_committedBytes.load(std::memory_order_relaxed);
	}
#endif // TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES

	void shutdown() noexcept;

	[[nodiscard]] void* allocSegments(std::size_t count, SegmentKind kind) noexcept;
	void freeSegments(void* segments, std::size_t count) noexcept;

	[[nodiscard]] std::size_t committedPages(const void* segment) const noexcept;

#if TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
	[[nodiscard]] bool enableLargePages() noexcept;
	void disableLargePages() noexcept;
#endif // TRIVIAL_MEMORY_ENABLE_LARGE_PAGES

	[[nodiscard]] bool ensureCommittedPages(void* segment, std::size_t pages, int& outOsErrorCode) noexcept;

#if TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
	[[nodiscard]] bool ensureCommittedLargePages(void* segment, std::size_t pages, int& outOsErrorCode) noexcept;
#endif // TRIVIAL_MEMORY_ENABLE_LARGE_PAGES

#if TRIVIAL_MEMORY_ENABLE_DECOMMIT
	void trimCommittedPagesTo(void* segment, std::size_t pages) noexcept;
#else
	void trimCommittedPagesTo(void* segment, std::size_t pages) noexcept {
		(void)segment;
		(void)pages;
	}
#endif // TRIVIAL_MEMORY_ENABLE_DECOMMIT

#if TRIVIAL_MEMORY_ENABLE_TICK
	void tick() noexcept;
#else
	void tick() noexcept {}
#endif // TRIVIAL_MEMORY_ENABLE_TICK

	[[nodiscard]] static void* segmentBase(void* ptr) noexcept {
		char* raw = static_cast<char*>(ptr);
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
		return raw - (reinterpret_cast<std::uintptr_t>(raw) & g_kSegmentMask);
	}

	[[nodiscard]] static const void* segmentBase(const void* ptr) noexcept {
		const char* raw = static_cast<const char*>(ptr);
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
		return raw - (reinterpret_cast<std::uintptr_t>(raw) & g_kSegmentMask);
	}

	[[nodiscard]] bool owns(const void* ptr) const noexcept;

	[[nodiscard]] const MemoryCapabilities& capabilities() const noexcept { return m_capabilities; }

	[[nodiscard]] std::size_t segmentCapacity() const noexcept { return m_segmentCapacity; }

	void setOomHandler(OomHandler handler) noexcept { m_oomHandler = handler; }

#if TRIVIAL_ENABLE_MEMORY_DEBUG_STATS
	[[nodiscard]] SegmentKind kindOf(const void* ptr) const noexcept;
	[[nodiscard]] SegmentRecord recordOf(const void* ptr) const noexcept;
	[[nodiscard]] std::size_t highWaterSegments() const noexcept { return m_highWaterSegments; }
#endif // TRIVIAL_ENABLE_MEMORY_DEBUG_STATS

private:
	void handleOom(std::size_t requestedSize, const char* context, int osErrorCode) const noexcept;

#if TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES
	[[nodiscard]] bool claimCommitBudget(std::size_t bytes) const noexcept;
	void releaseCommitBudget(std::size_t bytes) const noexcept;
#endif // TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES

#if TRIVIAL_MEMORY_ENABLE_DECOMMIT
	void purgeSegment(std::size_t segment) noexcept;
	void decommitRange(void* addr, std::size_t bytes) const noexcept;
#endif // TRIVIAL_MEMORY_ENABLE_DECOMMIT

	[[nodiscard]] std::size_t segmentIndex(const void* ptr) const noexcept;

	mutable std::mutex m_stateMutex;
	mutable std::mutex m_oomMutex;

#if TRIVIAL_PLATFORM_WINDOWS
	// For when windows does not allow for getting the base aligned, since it
	// may not be able to trim (not relevant to any real likely consumer, but
	// the cost is negligable makes the rare case of very old windows fail
	// gracefully)
	void* m_reservation = nullptr;
	std::size_t m_reservationBytes = 0;
#endif // TRIVIAL_PLATFORM_WINDOWS

	void* m_base = nullptr;
	std::size_t m_segmentCapacity = 0;

	std::uint64_t* m_allocatedBitmap = nullptr;
	std::uint64_t* m_cachedBitmap = nullptr;
#if TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
	// Segments backed by large pages can't be partially purged
	std::uint64_t* m_pinnedBitmap = nullptr;
#endif // TRIVIAL_MEMORY_ENABLE_LARGE_PAGES

	SegmentRecord* m_records = nullptr;

	// Metadata outside to protect from other stuff accidently overwriting
	void* m_metadata = nullptr;
	std::size_t m_metadataMappingBytes = 0;

	std::size_t m_highWaterSegments = 0;
	std::size_t m_cachedSegments = 0;

#if TRIVIAL_MEMORY_ENABLE_DECOMMIT
	std::size_t m_purgeCursor = 0;
#endif // TRIVIAL_MEMORY_ENABLE_DECOMMIT

#if TRIVIAL_MEMORY_ENABLE_TICK
	std::uint32_t m_tick = 0;
#endif // TRIVIAL_MEMORY_ENABLE_TICK

#if TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
	bool m_largePagesEnabled = false;
#endif // TRIVIAL_MEMORY_ENABLE_LARGE_PAGES

#if TRIVIAL_MEMORY_ENABLE_COMMIT_BUDGET && !TRIVIAL_MEMORY_FIXED_COMMIT_BUDGET
	std::size_t m_commitBudgetBytes = 0;
#endif // TRIVIAL_MEMORY_ENABLE_COMMIT_BUDGET && !TRIVIAL_MEMORY_FIXED_COMMIT_BUDGET

#if TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES
	mutable std::atomic<std::size_t> m_committedBytes{0};
#endif // TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES

	MemoryCapabilities m_capabilities{};

	OomHandler m_oomHandler = nullptr;
};

} // namespace trivial::memory

#endif // TRIVIAL_CORE_MEMORY_SEGMENT_ALLOCATOR_H

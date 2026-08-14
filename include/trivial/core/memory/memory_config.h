#ifndef TRIVIAL_CORE_MEMORY_MEMORY_CONFIG_H
#define TRIVIAL_CORE_MEMORY_MEMORY_CONFIG_H

#include <cstddef>
#include <cstdint>

#include <trivial/core/config.h>
#include <trivial/core/platform.h>

#ifndef TRIVIAL_ENABLE_MEMORY_DEBUG_STATS
#define TRIVIAL_ENABLE_MEMORY_DEBUG_STATS (TRIVIAL_CONFIG_DEBUG || TRIVIAL_CONFIG_RELWITHDEBINFO)
#endif

#ifndef TRIVIAL_ENABLE_MEMORY_DEBUG
#define TRIVIAL_ENABLE_MEMORY_DEBUG TRIVIAL_CONFIG_DEBUG
#endif

// NOTE: For use cases where returning memory has no benefit to the system,
//       here this being expected stuff for consoles or maybe stuff like a
//       deployment for a headless compute only operation running on a system
//       where your whole process gets a dedicated pool of physical memory
#ifndef TRIVIAL_MEMORY_ENABLE_DECOMMIT
#define TRIVIAL_MEMORY_ENABLE_DECOMMIT 1
#endif

#ifndef TRIVIAL_MEMORY_PREFER_LAZY_DECOMMIT
#define TRIVIAL_MEMORY_PREFER_LAZY_DECOMMIT (!TRIVIAL_ENABLE_MEMORY_DEBUG_STATS)
#endif

// NOTE: Not for regular usage only very specific well-informed workloads - will
//       be very unlikely to be advisable not to include in games
#ifndef TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
#define TRIVIAL_MEMORY_ENABLE_LARGE_PAGES 0
#endif

#ifndef TRIVIAL_MEMORY_COMMIT_BUDGET_BYTES
#define TRIVIAL_MEMORY_COMMIT_BUDGET_BYTES 0
#endif

#ifndef TRIVIAL_MEMORY_ENABLE_COMMIT_BUDGET
#if TRIVIAL_MEMORY_COMMIT_BUDGET_BYTES != 0
#define TRIVIAL_MEMORY_ENABLE_COMMIT_BUDGET 1
#else
#define TRIVIAL_MEMORY_ENABLE_COMMIT_BUDGET 0
#endif
#endif

#define TRIVIAL_MEMORY_FIXED_COMMIT_BUDGET                                                                             \
	(TRIVIAL_MEMORY_ENABLE_COMMIT_BUDGET && TRIVIAL_MEMORY_COMMIT_BUDGET_BYTES != 0)

#define TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES (TRIVIAL_MEMORY_ENABLE_COMMIT_BUDGET || TRIVIAL_ENABLE_MEMORY_DEBUG_STATS)

#define TRIVIAL_MEMORY_ENABLE_TICK (TRIVIAL_MEMORY_ENABLE_DECOMMIT || TRIVIAL_ENABLE_MEMORY_DEBUG_STATS)

// TODO: Tune shifts

#ifndef TRIVIAL_MEMORY_SEGMENT_SHIFT
#define TRIVIAL_MEMORY_SEGMENT_SHIFT 21 // 2 MiB
#endif

#ifndef TRIVIAL_MEMORY_SMALL_PAGE_SHIFT
#define TRIVIAL_MEMORY_SMALL_PAGE_SHIFT 16 // 64 KiB
#endif

#ifndef TRIVIAL_MEMORY_SMALL_MAX_SHIFT
#define TRIVIAL_MEMORY_SMALL_MAX_SHIFT 13
#endif

#ifndef TRIVIAL_MEMORY_MEDIUM_MAX_SHIFT
#define TRIVIAL_MEMORY_MEDIUM_MAX_SHIFT 19
#endif

#ifndef TRIVIAL_MEMORY_FRAMES_PER_TICK
#define TRIVIAL_MEMORY_FRAMES_PER_TICK 4
#endif

#ifndef TRIVIAL_MEMORY_DECAY_TICKS
#define TRIVIAL_MEMORY_DECAY_TICKS 15
#endif

#ifndef TRIVIAL_MEMORY_MIN_PURGE_PER_TICK
#define TRIVIAL_MEMORY_MIN_PURGE_PER_TICK 8
#endif

#ifndef TRIVIAL_MEMORY_PURGE_FRACTION
#define TRIVIAL_MEMORY_PURGE_FRACTION 8
#endif

#ifndef TRIVIAL_MEMORY_MAX_PURGE_PER_TICK
#define TRIVIAL_MEMORY_MAX_PURGE_PER_TICK 64
#endif

#ifndef TRIVIAL_MEMORY_MAX_CACHED_SEGMENTS
#define TRIVIAL_MEMORY_MAX_CACHED_SEGMENTS 64
#endif
#if (TRIVIAL_ENABLE_MEMORY_DEBUG_STATS != 0) && (TRIVIAL_ENABLE_MEMORY_DEBUG_STATS != 1)
#error "TRIVIAL_ENABLE_MEMORY_DEBUG_STATS must be 0 or 1"
#endif

#if (TRIVIAL_ENABLE_MEMORY_DEBUG != 0) && (TRIVIAL_ENABLE_MEMORY_DEBUG != 1)
#error "TRIVIAL_ENABLE_MEMORY_DEBUG must be 0 or 1"
#endif

#if (TRIVIAL_MEMORY_ENABLE_DECOMMIT != 0) && (TRIVIAL_MEMORY_ENABLE_DECOMMIT != 1)
#error "TRIVIAL_MEMORY_ENABLE_DECOMMIT must be 0 or 1"
#endif

#if (TRIVIAL_MEMORY_PREFER_LAZY_DECOMMIT != 0) && (TRIVIAL_MEMORY_PREFER_LAZY_DECOMMIT != 1)
#error "TRIVIAL_MEMORY_PREFER_LAZY_DECOMMIT must be 0 or 1"
#endif

#if (TRIVIAL_MEMORY_ENABLE_LARGE_PAGES != 0) && (TRIVIAL_MEMORY_ENABLE_LARGE_PAGES != 1)
#error "TRIVIAL_MEMORY_ENABLE_LARGE_PAGES must be 0 or 1"
#endif

#if (TRIVIAL_MEMORY_ENABLE_COMMIT_BUDGET != 0) && (TRIVIAL_MEMORY_ENABLE_COMMIT_BUDGET != 1)
#error "TRIVIAL_MEMORY_ENABLE_COMMIT_BUDGET must be 0 or 1"
#endif

namespace trivial::memory {

inline constexpr std::size_t g_kSegmentShift = TRIVIAL_MEMORY_SEGMENT_SHIFT;
inline constexpr std::size_t g_kSegmentSize = std::size_t{1} << g_kSegmentShift;
inline constexpr std::size_t g_kSegmentMask = g_kSegmentSize - 1;

inline constexpr std::size_t g_kSmallPageShift = TRIVIAL_MEMORY_SMALL_PAGE_SHIFT;
inline constexpr std::size_t g_kSmallPageSize = std::size_t{1} << g_kSmallPageShift;
inline constexpr std::size_t g_kSmallPageMask = g_kSmallPageSize - 1;
inline constexpr std::size_t g_kSmallPagesPerSegment = g_kSegmentSize >> g_kSmallPageShift;

inline constexpr std::size_t g_kSmallMaxSize = std::size_t{1} << TRIVIAL_MEMORY_SMALL_MAX_SHIFT;
inline constexpr std::size_t g_kMediumMaxSize = std::size_t{1} << TRIVIAL_MEMORY_MEDIUM_MAX_SHIFT;

inline constexpr std::uint32_t g_kFramesPerTick = TRIVIAL_MEMORY_FRAMES_PER_TICK;
inline constexpr std::uint32_t g_kDecayTicks = TRIVIAL_MEMORY_DECAY_TICKS;
inline constexpr std::size_t g_kMinPurgePerTick = TRIVIAL_MEMORY_MIN_PURGE_PER_TICK;
inline constexpr std::size_t g_kPurgeFraction = TRIVIAL_MEMORY_PURGE_FRACTION;
inline constexpr std::size_t g_kMaxPurgePerTick = TRIVIAL_MEMORY_MAX_PURGE_PER_TICK;
inline constexpr std::size_t g_kMaxCachedSegments = TRIVIAL_MEMORY_MAX_CACHED_SEGMENTS;

#if TRIVIAL_MEMORY_FIXED_COMMIT_BUDGET
inline constexpr std::size_t g_kCommitBudgetBytes = TRIVIAL_MEMORY_COMMIT_BUDGET_BYTES;

static_assert(g_kCommitBudgetBytes >= g_kSegmentSize, "Commit budget below a single segment");
#endif // TRIVIAL_MEMORY_FIXED_COMMIT_BUDGET

// NOLINTNEXTLINE(readability-magic-numbers)
static_assert(g_kSegmentShift >= 16 && g_kSegmentShift <= 30, "Segment size outside sane range");
static_assert(g_kSmallPageShift < g_kSegmentShift, "Small pages must be smaller than a segment");
static_assert(g_kSmallPagesPerSegment >= 4, "Too few small pages per segment to be worth sharding");
static_assert(g_kSmallMaxSize * 4 <= g_kSmallPageSize, "Small tier needs at least four blocks per page");
static_assert(g_kMediumMaxSize * 4 <= g_kSegmentSize, "Medium tier needs several blocks per segment to drain");
static_assert(g_kSmallMaxSize < g_kMediumMaxSize, "Tier boundaries out of order");
static_assert(g_kFramesPerTick > 0, "Frames per tick must be non zero");
static_assert(g_kPurgeFraction > 0, "Purge fraction must be non zero");
static_assert(g_kMaxPurgePerTick >= g_kMinPurgePerTick, "Purge ceiling below its floor");

#if TRIVIAL_PLATFORM_PAGE_SIZE_KNOWN
static_assert(g_kSegmentSize % core::g_kPageSize == 0, "Segment size must be a whole number of pages");
static_assert(g_kSmallPageSize % core::g_kPageSize == 0, "Small page size must be a whole number of OS pages");
static_assert(g_kSegmentSize >= core::g_kAllocationGranularity,
              "Segments must be at least the reservation granularity");
#endif // TRIVIAL_PLATFORM_PAGE_SIZE_KNOWN

} // namespace trivial::memory

#endif // TRIVIAL_CORE_MEMORY_MEMORY_CONFIG_H

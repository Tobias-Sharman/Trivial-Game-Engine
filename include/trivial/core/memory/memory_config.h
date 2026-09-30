#ifndef TRIVIAL_CORE_MEMORY_MEMORY_CONFIG_H
#define TRIVIAL_CORE_MEMORY_MEMORY_CONFIG_H

#include <cstddef>

#include <trivial/core/config.h> // IWYU pragma: export
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
#define TRIVIAL_MEMORY_PREFER_LAZY_DECOMMIT 0
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

#define TRIVIAL_MEMORY_LAZY_DECOMMIT                                                                                   \
	(TRIVIAL_MEMORY_ENABLE_DECOMMIT && TRIVIAL_MEMORY_PREFER_LAZY_DECOMMIT                                             \
	 && (TRIVIAL_PLATFORM_WINDOWS || TRIVIAL_PLATFORM_SDK_HAS_MADV_FREE))

// TODO: Tune shifts

#ifndef TRIVIAL_MEMORY_SEGMENT_SHIFT
#define TRIVIAL_MEMORY_SEGMENT_SHIFT 21U // 2 MiB
#endif

#ifndef TRIVIAL_MEMORY_SMALL_PAGE_SHIFT
#define TRIVIAL_MEMORY_SMALL_PAGE_SHIFT 16U // 64 KiB
#endif

#ifndef TRIVIAL_MEMORY_SMALL_MAX_SHIFT
#define TRIVIAL_MEMORY_SMALL_MAX_SHIFT 13U
#endif

#ifndef TRIVIAL_MEMORY_MEDIUM_MAX_SHIFT
#define TRIVIAL_MEMORY_MEDIUM_MAX_SHIFT 19U
#endif

#ifndef TRIVIAL_MEMORY_FRAMES_PER_TICK
#define TRIVIAL_MEMORY_FRAMES_PER_TICK 4U
#endif

#ifndef TRIVIAL_MEMORY_DECAY_TICKS
#define TRIVIAL_MEMORY_DECAY_TICKS 15U
#endif

#ifndef TRIVIAL_MEMORY_MIN_PURGE_PER_TICK
#define TRIVIAL_MEMORY_MIN_PURGE_PER_TICK 8U
#endif

#ifndef TRIVIAL_MEMORY_PURGE_FRACTION
#define TRIVIAL_MEMORY_PURGE_FRACTION 8U
#endif

#ifndef TRIVIAL_MEMORY_MAX_PURGE_PER_TICK
#define TRIVIAL_MEMORY_MAX_PURGE_PER_TICK 64U
#endif

#ifndef TRIVIAL_MEMORY_MAX_CACHED_SEGMENTS
#define TRIVIAL_MEMORY_MAX_CACHED_SEGMENTS 64U
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

#define TRIVIAL_MEMORY_SEGMENT_SIZE (std::size_t{1} << TRIVIAL_MEMORY_SEGMENT_SHIFT)
#define TRIVIAL_MEMORY_SEGMENT_MASK (TRIVIAL_MEMORY_SEGMENT_SIZE - std::size_t{1})

#define TRIVIAL_MEMORY_SMALL_PAGE_SIZE (std::size_t{1} << TRIVIAL_MEMORY_SMALL_PAGE_SHIFT)
#define TRIVIAL_MEMORY_SMALL_PAGE_MASK (TRIVIAL_MEMORY_SMALL_PAGE_SIZE - std::size_t{1})
#define TRIVIAL_MEMORY_SMALL_PAGES_PER_SEGMENT (TRIVIAL_MEMORY_SEGMENT_SIZE >> TRIVIAL_MEMORY_SMALL_PAGE_SHIFT)

#define TRIVIAL_MEMORY_SMALL_MAX_SIZE (std::size_t{1} << TRIVIAL_MEMORY_SMALL_MAX_SHIFT)
#define TRIVIAL_MEMORY_MEDIUM_MAX_SIZE (std::size_t{1} << TRIVIAL_MEMORY_MEDIUM_MAX_SHIFT)

#if TRIVIAL_MEMORY_FIXED_COMMIT_BUDGET
static_assert(TRIVIAL_MEMORY_COMMIT_BUDGET_BYTES >= TRIVIAL_MEMORY_SEGMENT_SIZE,
              "Commit budget below a single segment");
#endif // TRIVIAL_MEMORY_FIXED_COMMIT_BUDGET

// NOLINTNEXTLINE(readability-magic-numbers)
static_assert(TRIVIAL_MEMORY_SEGMENT_SHIFT >= 16 && TRIVIAL_MEMORY_SEGMENT_SHIFT <= 30,
              "Segment size outside sane range");
static_assert(TRIVIAL_MEMORY_SMALL_PAGE_SHIFT < TRIVIAL_MEMORY_SEGMENT_SHIFT,
              "Small pages must be smaller than a segment");
static_assert(TRIVIAL_MEMORY_SMALL_PAGES_PER_SEGMENT >= 4, "Too few small pages per segment to be worth sharding");
static_assert(TRIVIAL_MEMORY_SMALL_MAX_SIZE * 4 <= TRIVIAL_MEMORY_SMALL_PAGE_SIZE,
              "Small tier needs at least four blocks per page");
static_assert(TRIVIAL_MEMORY_MEDIUM_MAX_SIZE * 4 <= TRIVIAL_MEMORY_SEGMENT_SIZE,
              "Medium tier needs several blocks per segment to drain");
static_assert(TRIVIAL_MEMORY_SMALL_MAX_SIZE < TRIVIAL_MEMORY_MEDIUM_MAX_SIZE, "Tier boundaries out of order");
static_assert(TRIVIAL_MEMORY_FRAMES_PER_TICK > 0, "Frames per tick must be non zero");
static_assert(TRIVIAL_MEMORY_PURGE_FRACTION > 0, "Purge fraction must be non zero");
static_assert(TRIVIAL_MEMORY_MAX_PURGE_PER_TICK >= TRIVIAL_MEMORY_MIN_PURGE_PER_TICK, "Purge ceiling below its floor");

#if TRIVIAL_PLATFORM_PAGE_SIZE_KNOWN
static_assert(TRIVIAL_MEMORY_SEGMENT_SIZE % TRIVIAL_PLATFORM_PAGE_SIZE == 0,
              "Segment size must be a whole number of pages");
static_assert(TRIVIAL_MEMORY_SMALL_PAGE_SIZE % TRIVIAL_PLATFORM_PAGE_SIZE == 0,
              "Small page size must be a whole number of OS pages");
static_assert(TRIVIAL_MEMORY_SEGMENT_SIZE >= TRIVIAL_PLATFORM_ALLOCATION_GRANULARITY,
              "Segments must be at least the reservation granularity");
#endif // TRIVIAL_PLATFORM_PAGE_SIZE_KNOWN

#endif // TRIVIAL_CORE_MEMORY_MEMORY_CONFIG_H

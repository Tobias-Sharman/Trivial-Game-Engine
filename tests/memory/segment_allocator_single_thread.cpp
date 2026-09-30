#include <cstdint>
#include <cstring>
#include <vector>

#include <gtest/gtest.h>

#include <trivial/core/memory/memory_config.h>
#include <trivial/core/memory/oom_handler.h>

#include "core/memory/memory_capabilities.h"
#include "core/memory/segment_allocator.h"

using namespace trivial::memory;

namespace {

constexpr std::size_t g_kTestSegments = 32;
constexpr std::size_t g_kTestReserve = g_kTestSegments * TRIVIAL_MEMORY_SEGMENT_SIZE;

[[nodiscard]] std::size_t segmentOffset(const SegmentAllocator& allocator, const void* base, const void* ptr) {
	(void)allocator;
	return (static_cast<const char*>(ptr) - static_cast<const char*>(base)) / TRIVIAL_MEMORY_SEGMENT_SIZE;
}

class SegmentAllocatorSingleThreadTest : public ::testing::Test {
protected:
	void SetUp() override { ASSERT_TRUE(m_allocator.init(g_kTestReserve)); }

	void TearDown() override { m_allocator.shutdown(); }

	SegmentAllocator m_allocator;
};

// -----------------------------------------------------------------------------
// Reservation and alignment
// -----------------------------------------------------------------------------

TEST_F(SegmentAllocatorSingleThreadTest, ReservationRoundsUpToSegments) {
	EXPECT_GE(m_allocator.segmentCapacity(), g_kTestSegments);
}

TEST(SegmentAllocatorSingleThreadStandalone, UnalignedReserveRoundsUp) {
	SegmentAllocator allocator;
	ASSERT_TRUE(allocator.init(TRIVIAL_MEMORY_SEGMENT_SIZE + 1));
	EXPECT_EQ(allocator.segmentCapacity(), 2U);
	allocator.shutdown();
}

TEST_F(SegmentAllocatorSingleThreadTest, SegmentsAreSegmentAligned) {
	void* segment = m_allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_NE(segment, nullptr);
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	EXPECT_EQ(reinterpret_cast<std::uintptr_t>(segment) & TRIVIAL_MEMORY_SEGMENT_MASK, 0U);
	m_allocator.freeSegments(segment, 1);
}

TEST_F(SegmentAllocatorSingleThreadTest, SegmentBaseMasksInteriorPointers) {
	void* segment = m_allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_NE(segment, nullptr);

	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	char* interior = static_cast<char*>(segment) + (TRIVIAL_MEMORY_SEGMENT_SIZE / 2);
	EXPECT_EQ(SegmentAllocator::segmentBase(interior), segment);

	m_allocator.freeSegments(segment, 1);
}

// -----------------------------------------------------------------------------
// Ownership
// -----------------------------------------------------------------------------

TEST_F(SegmentAllocatorSingleThreadTest, OwnsRejectsOutsidePointers) {
	int stackValue = 0;
	EXPECT_FALSE(m_allocator.owns(&stackValue));
	EXPECT_FALSE(m_allocator.owns(nullptr));
}

TEST_F(SegmentAllocatorSingleThreadTest, OwnsAcceptsInteriorPointers) {
	void* segment = m_allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_NE(segment, nullptr);

	EXPECT_TRUE(m_allocator.owns(segment));
	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	EXPECT_TRUE(m_allocator.owns(static_cast<char*>(segment) + TRIVIAL_MEMORY_SEGMENT_SIZE - 1));

	m_allocator.freeSegments(segment, 1);
}

TEST_F(SegmentAllocatorSingleThreadTest, OwnsRejectsOnePastReservationEnd) {
	std::vector<void*> held;

	// Sequential single-segment allocations on a fresh allocator land at
	// ascending indices, so the last one handed out sits at the top of the
	// reservation
	while (void* segment = m_allocator.allocSegments(1, SegmentKind::Small)) {
		held.push_back(segment);
	}

	ASSERT_FALSE(held.empty());

	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	const char* kOnePastEnd = static_cast<char*>(held.back()) + TRIVIAL_MEMORY_SEGMENT_SIZE;

	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	EXPECT_TRUE(m_allocator.owns(static_cast<char*>(held.back()) + TRIVIAL_MEMORY_SEGMENT_SIZE - 1));
	EXPECT_FALSE(m_allocator.owns(kOnePastEnd));

	for (void* segment : held) {
		m_allocator.freeSegments(segment, 1);
	}
}

// -----------------------------------------------------------------------------
// Allocation and run finding
// -----------------------------------------------------------------------------

TEST_F(SegmentAllocatorSingleThreadTest, AllocatesDistinctSegments) {
	void* first = m_allocator.allocSegments(1, SegmentKind::Small);
	void* second = m_allocator.allocSegments(1, SegmentKind::Small);

	ASSERT_NE(first, nullptr);
	ASSERT_NE(second, nullptr);
	EXPECT_NE(first, second);

	m_allocator.freeSegments(first, 1);
	m_allocator.freeSegments(second, 1);
}

TEST_F(SegmentAllocatorSingleThreadTest, MultiSegmentRunIsContiguous) {
	constexpr std::size_t kRunSegments = 4;

	void* run = m_allocator.allocSegments(kRunSegments, SegmentKind::HugeHead);
	ASSERT_NE(run, nullptr);

	// Segments only reserve address space, so each one needs its own commit
	// before it can be written
	const std::size_t kPagesPerSegment = TRIVIAL_MEMORY_SEGMENT_SIZE / pageSize();
	int error = 0;

	for (std::size_t segment = 0; segment < kRunSegments; ++segment) {
		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
		void* segmentBase = static_cast<char*>(run) + (segment * TRIVIAL_MEMORY_SEGMENT_SIZE);
		ASSERT_TRUE(m_allocator.ensureCommittedPages(segmentBase, kPagesPerSegment, error));
	}

	// Writing across the whole run proves the address space is one contiguous range
	std::memset(run, 0xCD, kRunSegments * TRIVIAL_MEMORY_SEGMENT_SIZE);

	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	EXPECT_EQ(static_cast<unsigned char*>(run)[(kRunSegments * TRIVIAL_MEMORY_SEGMENT_SIZE) - 1], 0xCD);

	m_allocator.freeSegments(run, kRunSegments);
}

TEST_F(SegmentAllocatorSingleThreadTest, ExhaustionReturnsNullptr) {
	std::vector<void*> held;

	while (void* segment = m_allocator.allocSegments(1, SegmentKind::Small)) {
		held.push_back(segment);
	}

	EXPECT_FALSE(held.empty());
	EXPECT_EQ(m_allocator.allocSegments(1, SegmentKind::Small), nullptr);

	for (void* segment : held) {
		m_allocator.freeSegments(segment, 1);
	}
}

TEST_F(SegmentAllocatorSingleThreadTest, RunLargerThanCapacityFails) {
	EXPECT_EQ(m_allocator.allocSegments(m_allocator.segmentCapacity() + 1, SegmentKind::HugeHead), nullptr);
}

TEST_F(SegmentAllocatorSingleThreadTest, FragmentationBlocksContiguousRun) {
	std::vector<void*> held;

	while (void* segment = m_allocator.allocSegments(1, SegmentKind::Small)) {
		held.push_back(segment);
	}

	ASSERT_GE(held.size(), 4U);

	// Free alternating segments, so plenty is free but nothing is adjacent
	for (std::size_t index = 0; index < held.size(); index += 2) {
		m_allocator.freeSegments(held[index], 1);
		held[index] = nullptr;
	}

	EXPECT_EQ(m_allocator.allocSegments(2, SegmentKind::HugeHead), nullptr);

	for (void* segment : held) {
		if (segment != nullptr) {
			m_allocator.freeSegments(segment, 1);
		}
	}
}

TEST_F(SegmentAllocatorSingleThreadTest, FreedRunIsReusable) {
	void* first = m_allocator.allocSegments(3, SegmentKind::HugeHead);
	ASSERT_NE(first, nullptr);
	m_allocator.freeSegments(first, 3);

	void* second = m_allocator.allocSegments(3, SegmentKind::HugeHead);
	EXPECT_EQ(second, first);
	m_allocator.freeSegments(second, 3);
}

TEST_F(SegmentAllocatorSingleThreadTest, AdjacentFreedSegmentsCoalesce) {
	void* first = m_allocator.allocSegments(1, SegmentKind::Small);
	void* second = m_allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_NE(first, nullptr);
	ASSERT_NE(second, nullptr);

	m_allocator.freeSegments(first, 1);
	m_allocator.freeSegments(second, 1);

	// Adjacency in the bitmap is adjacency in memory, so no work is needed for
	// two singles to satisfy a run of two
	void* run = m_allocator.allocSegments(2, SegmentKind::HugeHead);
	EXPECT_NE(run, nullptr);

	if (run != nullptr) {
		m_allocator.freeSegments(run, 2);
	}
}

// -----------------------------------------------------------------------------
// Commit prefix
// -----------------------------------------------------------------------------

TEST_F(SegmentAllocatorSingleThreadTest, FreshSegmentHasNoCommittedPages) {
	void* segment = m_allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_NE(segment, nullptr);

	EXPECT_EQ(m_allocator.committedPages(segment), 0U);

	m_allocator.freeSegments(segment, 1);
}

TEST_F(SegmentAllocatorSingleThreadTest, CommitGrowsPrefixAndMemoryIsWritable) {
	void* segment = m_allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_NE(segment, nullptr);

	int error = 0;
	ASSERT_TRUE(m_allocator.ensureCommittedPages(segment, 4, error));
	EXPECT_EQ(m_allocator.committedPages(segment), 4U);

	const std::size_t kBytes = 4 * pageSize();
	std::memset(segment, 0xEF, kBytes);
	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	EXPECT_EQ(static_cast<unsigned char*>(segment)[kBytes - 1], 0xEF);

	m_allocator.freeSegments(segment, 1);
}

TEST_F(SegmentAllocatorSingleThreadTest, CommitIsIdempotent) {
	void* segment = m_allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_NE(segment, nullptr);

	int error = 0;
	ASSERT_TRUE(m_allocator.ensureCommittedPages(segment, 8, error));
	ASSERT_TRUE(m_allocator.ensureCommittedPages(segment, 8, error));
	ASSERT_TRUE(m_allocator.ensureCommittedPages(segment, 4, error));

	// The prefix is a target, not an increment, and it never shrinks on commit
	EXPECT_EQ(m_allocator.committedPages(segment), 8U);

	m_allocator.freeSegments(segment, 1);
}

TEST_F(SegmentAllocatorSingleThreadTest, CommitOnlyGrowsTheDelta) {
	void* segment = m_allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_NE(segment, nullptr);

	int error = 0;
	ASSERT_TRUE(m_allocator.ensureCommittedPages(segment, 4, error));
	ASSERT_TRUE(m_allocator.ensureCommittedPages(segment, 10, error));
	EXPECT_EQ(m_allocator.committedPages(segment), 10U);

	// Pages committed by the first call must still be writable after the second
	std::memset(segment, 0x11, 10 * pageSize());

	m_allocator.freeSegments(segment, 1);
}

#if TRIVIAL_MEMORY_ENABLE_COMMIT_BUDGET && !TRIVIAL_MEMORY_FIXED_COMMIT_BUDGET
TEST_F(SegmentAllocatorSingleThreadTest, CommitBudgetRejectsOverBudgetCommits) {
	static bool s_oomFired = false;
	s_oomFired = false;

	const std::size_t kPageSize = pageSize();
	m_allocator.setCommitBudget(2 * kPageSize);

	void* segment = m_allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_NE(segment, nullptr);

	int error = 0;
	ASSERT_TRUE(m_allocator.ensureCommittedPages(segment, 2, error));

	m_allocator.setOomHandler([](const OomInfo& info) {
		(void)info;
		s_oomFired = true;
	});

	// Growing the prefix by even one more page would exceed the budget
	EXPECT_FALSE(m_allocator.ensureCommittedPages(segment, 3, error));
	EXPECT_TRUE(s_oomFired);
	EXPECT_EQ(m_allocator.committedPages(segment), 2U);

	m_allocator.freeSegments(segment, 1);
}
#endif // TRIVIAL_MEMORY_ENABLE_COMMIT_BUDGET && !TRIVIAL_MEMORY_FIXED_COMMIT_BUDGET

TEST_F(SegmentAllocatorSingleThreadTest, CommitFillsEntireSegment) {
	void* segment = m_allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_NE(segment, nullptr);

	const std::size_t kPagesPerSegment = TRIVIAL_MEMORY_SEGMENT_SIZE / pageSize();

	int error = 0;
	ASSERT_TRUE(m_allocator.ensureCommittedPages(segment, kPagesPerSegment, error));
	EXPECT_EQ(m_allocator.committedPages(segment), kPagesPerSegment);

	// The full segment must be writable right up to its last byte
	std::memset(segment, 0x77, TRIVIAL_MEMORY_SEGMENT_SIZE);
	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	EXPECT_EQ(static_cast<unsigned char*>(segment)[TRIVIAL_MEMORY_SEGMENT_SIZE - 1], 0x77);

	m_allocator.freeSegments(segment, 1);
}

TEST_F(SegmentAllocatorSingleThreadTest, CommittedPagesRejectsForeignPointers) {
	int stackValue = 0;
	EXPECT_EQ(m_allocator.committedPages(&stackValue), 0U);
}

// -----------------------------------------------------------------------------
// Cache and adoption
// -----------------------------------------------------------------------------

TEST_F(SegmentAllocatorSingleThreadTest, FreedSegmentKeepsCommittedPrefix) {
	void* segment = m_allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_NE(segment, nullptr);

	int error = 0;
	ASSERT_TRUE(m_allocator.ensureCommittedPages(segment, 6, error));
	m_allocator.freeSegments(segment, 1);

	void* adopted = m_allocator.allocSegments(1, SegmentKind::Medium);
	ASSERT_EQ(adopted, segment);
	EXPECT_EQ(m_allocator.committedPages(adopted), 6U);

	m_allocator.freeSegments(adopted, 1);
}

TEST_F(SegmentAllocatorSingleThreadTest, CachedSegmentIsPreferredOverFreshOne) {
	void* first = m_allocator.allocSegments(1, SegmentKind::Small);
	void* second = m_allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_NE(first, nullptr);
	ASSERT_NE(second, nullptr);

	int error = 0;
	ASSERT_TRUE(m_allocator.ensureCommittedPages(second, 2, error));

	// first is released with nothing committed, second with a live prefix, so
	// only second enters the cache and should win despite the lower index
	m_allocator.freeSegments(first, 1);
	m_allocator.freeSegments(second, 1);

	void* adopted = m_allocator.allocSegments(1, SegmentKind::Small);
	EXPECT_EQ(adopted, second);

	m_allocator.freeSegments(adopted, 1);
}

#if TRIVIAL_MEMORY_ENABLE_DECOMMIT
TEST_F(SegmentAllocatorSingleThreadTest, TrimShrinksPrefix) {
	void* segment = m_allocator.allocSegments(1, SegmentKind::ScratchArena);
	ASSERT_NE(segment, nullptr);

	int error = 0;
	ASSERT_TRUE(m_allocator.ensureCommittedPages(segment, 10, error));

	m_allocator.trimCommittedPagesTo(segment, 4);
	EXPECT_EQ(m_allocator.committedPages(segment), 4U);

	// The surviving prefix must still be backed
	std::memset(segment, 0x22, 4 * pageSize());

	m_allocator.freeSegments(segment, 1);
}

TEST_F(SegmentAllocatorSingleThreadTest, TrimAboveCurrentPrefixDoesNothing) {
	void* segment = m_allocator.allocSegments(1, SegmentKind::ScratchArena);
	ASSERT_NE(segment, nullptr);

	int error = 0;
	ASSERT_TRUE(m_allocator.ensureCommittedPages(segment, 4, error));

	m_allocator.trimCommittedPagesTo(segment, 8);
	EXPECT_EQ(m_allocator.committedPages(segment), 4U);

	m_allocator.freeSegments(segment, 1);
}

TEST_F(SegmentAllocatorSingleThreadTest, RecommitAfterTrimIsWritable) {
	void* segment = m_allocator.allocSegments(1, SegmentKind::ScratchArena);
	ASSERT_NE(segment, nullptr);

	int error = 0;
	ASSERT_TRUE(m_allocator.ensureCommittedPages(segment, 8, error));
	m_allocator.trimCommittedPagesTo(segment, 2);
	ASSERT_TRUE(m_allocator.ensureCommittedPages(segment, 8, error));

	std::memset(segment, 0x33, 8 * pageSize());
	EXPECT_EQ(m_allocator.committedPages(segment), 8U);

	m_allocator.freeSegments(segment, 1);
}
#endif // TRIVIAL_MEMORY_ENABLE_DECOMMIT

// -----------------------------------------------------------------------------
// Decay and purge
// -----------------------------------------------------------------------------

#if TRIVIAL_MEMORY_ENABLE_DECOMMIT
TEST_F(SegmentAllocatorSingleThreadTest, DecayPurgesCachedSegments) {
	void* segment = m_allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_NE(segment, nullptr);

	int error = 0;
	ASSERT_TRUE(m_allocator.ensureCommittedPages(segment, 4, error));
	m_allocator.freeSegments(segment, 1);

	for (std::uint32_t tick = 0; tick <= TRIVIAL_MEMORY_DECAY_TICKS + 1; ++tick) {
		m_allocator.tick();
	}

	void* adopted = m_allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_EQ(adopted, segment);
	EXPECT_EQ(m_allocator.committedPages(adopted), 0U);

	m_allocator.freeSegments(adopted, 1);
}

TEST_F(SegmentAllocatorSingleThreadTest, CachedSegmentSurvivesInsideDecayWindow) {
	void* segment = m_allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_NE(segment, nullptr);

	int error = 0;
	ASSERT_TRUE(m_allocator.ensureCommittedPages(segment, 4, error));
	m_allocator.freeSegments(segment, 1);

	m_allocator.tick();

	void* adopted = m_allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_EQ(adopted, segment);
	EXPECT_EQ(m_allocator.committedPages(adopted), 4U);

	m_allocator.freeSegments(adopted, 1);
}

TEST(SegmentAllocatorSingleThreadStandalone, CacheOverflowPurgesInsteadOfCaching) {
	constexpr std::size_t kSegments = TRIVIAL_MEMORY_MAX_CACHED_SEGMENTS + 2;

	SegmentAllocator allocator;
	ASSERT_TRUE(allocator.init(kSegments * TRIVIAL_MEMORY_SEGMENT_SIZE));

	std::vector<void*> held;
	held.reserve(TRIVIAL_MEMORY_MAX_CACHED_SEGMENTS + 1);

	for (std::size_t i = 0; i < TRIVIAL_MEMORY_MAX_CACHED_SEGMENTS + 1; ++i) {
		void* segment = allocator.allocSegments(1, SegmentKind::Small);
		ASSERT_NE(segment, nullptr);

		int error = 0;
		ASSERT_TRUE(allocator.ensureCommittedPages(segment, 2, error));

		held.push_back(segment);
	}

	// The cache only holds TRIVIAL_MEMORY_MAX_CACHED_SEGMENTS entries, so freeing one more
	// than that must purge the overflow immediately instead of caching it
	for (void* segment : held) {
		allocator.freeSegments(segment, 1);
	}

	std::vector<void*> reclaimed;
	reclaimed.reserve(TRIVIAL_MEMORY_MAX_CACHED_SEGMENTS);

	for (std::size_t i = 0; i < TRIVIAL_MEMORY_MAX_CACHED_SEGMENTS; ++i) {
		void* segment = allocator.allocSegments(1, SegmentKind::Small);
		ASSERT_NE(segment, nullptr);
		EXPECT_EQ(allocator.committedPages(segment), 2U);
		reclaimed.push_back(segment);
	}

	// Every cache slot has been drained, so this can only be the segment that
	// overflowed the cache and was purged on free
	void* overflowed = allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_NE(overflowed, nullptr);
	EXPECT_EQ(allocator.committedPages(overflowed), 0U);

	allocator.freeSegments(overflowed, 1);

	for (void* segment : reclaimed) {
		allocator.freeSegments(segment, 1);
	}

	allocator.shutdown();
}
#endif // TRIVIAL_MEMORY_ENABLE_DECOMMIT

// -----------------------------------------------------------------------------
// Stats
// -----------------------------------------------------------------------------

#if TRIVIAL_ENABLE_MEMORY_DEBUG_STATS
TEST_F(SegmentAllocatorSingleThreadTest, KindIsRecordedAndCleared) {
	void* segment = m_allocator.allocSegments(1, SegmentKind::EcsChunks);
	ASSERT_NE(segment, nullptr);

	EXPECT_EQ(m_allocator.kindOf(segment), SegmentKind::EcsChunks);
	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	EXPECT_EQ(m_allocator.kindOf(static_cast<char*>(segment) + 128), SegmentKind::EcsChunks);

	m_allocator.freeSegments(segment, 1);
	EXPECT_EQ(m_allocator.kindOf(segment), SegmentKind::Invalid);
}

TEST_F(SegmentAllocatorSingleThreadTest, KindOfForeignPointerIsInvalid) {
	int stackValue = 0;
	EXPECT_EQ(m_allocator.kindOf(&stackValue), SegmentKind::Invalid);
}

TEST_F(SegmentAllocatorSingleThreadTest, RecordOfReflectsKindAndCommittedPages) {
	void* segment = m_allocator.allocSegments(1, SegmentKind::TexturePool);
	ASSERT_NE(segment, nullptr);

	int error = 0;
	ASSERT_TRUE(m_allocator.ensureCommittedPages(segment, 3, error));

	const SegmentRecord kRecord = m_allocator.recordOf(segment);
	EXPECT_EQ(kRecord.kind, SegmentKind::TexturePool);
	EXPECT_EQ(kRecord.committedPages, 3U);
	EXPECT_EQ(kRecord.runLength, 1U);

	m_allocator.freeSegments(segment, 1);

	int stackValue = 0;
	EXPECT_EQ(m_allocator.recordOf(&stackValue).kind, SegmentKind::Invalid);
}

TEST_F(SegmentAllocatorSingleThreadTest, HighWaterOnlyGrows) {
	void* first = m_allocator.allocSegments(2, SegmentKind::Small);
	ASSERT_NE(first, nullptr);

	const std::size_t kAfterAlloc = m_allocator.highWaterSegments();
	EXPECT_GE(kAfterAlloc, 2U);

	m_allocator.freeSegments(first, 2);
	EXPECT_EQ(m_allocator.highWaterSegments(), kAfterAlloc);
}
#endif // TRIVIAL_ENABLE_MEMORY_DEBUG_STATS

#if TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES
TEST_F(SegmentAllocatorSingleThreadTest, CommittedBytesTracksCommits) {
	const std::size_t kBefore = m_allocator.committedBytes();

	void* segment = m_allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_NE(segment, nullptr);

	int error = 0;
	ASSERT_TRUE(m_allocator.ensureCommittedPages(segment, 4, error));

	const std::size_t kPageSize = pageSize();
	EXPECT_EQ(m_allocator.committedBytes(), kBefore + (4 * kPageSize));

	m_allocator.freeSegments(segment, 1);
}
#endif // TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES

// -----------------------------------------------------------------------------
// Lifecycle
// -----------------------------------------------------------------------------

TEST(SegmentAllocatorSingleThreadStandalone, FailedInitLeavesStateClean) {
	static bool s_handlerFired = false;
	s_handlerFired = false;

	SegmentAllocator allocator;
	allocator.setOomHandler([](const OomInfo& info) {
		(void)info;
		s_handlerFired = true;
	});

	EXPECT_FALSE(allocator.init(SIZE_MAX / 2));
	EXPECT_TRUE(s_handlerFired);

	// A failed init must not leave a mapping or partial state behind
	EXPECT_TRUE(allocator.init(g_kTestReserve));
	EXPECT_NE(allocator.allocSegments(1, SegmentKind::Small), nullptr);

	allocator.shutdown();
}

TEST(SegmentAllocatorSingleThreadStandalone, ShutdownIsIdempotent) {
	SegmentAllocator allocator;
	ASSERT_TRUE(allocator.init(g_kTestReserve));

	allocator.shutdown();
	allocator.shutdown();
}

TEST(SegmentAllocatorSingleThreadStandalone, OomHandlerReportsRequestedSize) {
	static std::size_t s_capturedSize = 0;
	s_capturedSize = 0;

	SegmentAllocator allocator;
	ASSERT_TRUE(allocator.init(TRIVIAL_MEMORY_SEGMENT_SIZE));

	allocator.setOomHandler([](const OomInfo& info) {
		s_capturedSize = info.requestedSize;
	});

	EXPECT_EQ(allocator.allocSegments(4, SegmentKind::HugeHead), nullptr);
	EXPECT_EQ(s_capturedSize, 4 * TRIVIAL_MEMORY_SEGMENT_SIZE);

	allocator.shutdown();
}

} // namespace

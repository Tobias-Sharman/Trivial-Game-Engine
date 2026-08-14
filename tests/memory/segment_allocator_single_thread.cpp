#include <cstring>
#include <vector>

#include <gtest/gtest.h>

#include <trivial/core/memory/segment_allocator.h>

using namespace trivial::memory;

namespace {

constexpr std::size_t kTestSegments = 32;
constexpr std::size_t kTestReserve = kTestSegments * g_kSegmentSize;

[[nodiscard]] std::size_t segmentOffset(const SegmentAllocator& allocator, const void* base, const void* ptr) {
	(void)allocator;
	return (static_cast<const char*>(ptr) - static_cast<const char*>(base)) / g_kSegmentSize;
}

class SegmentAllocatorSingleThreadTest : public ::testing::Test {
protected:
	void SetUp() override { ASSERT_TRUE(allocator.init(kTestReserve)); }

	void TearDown() override { allocator.shutdown(); }

	SegmentAllocator allocator;
};

// -----------------------------------------------------------------------------
// Reservation and alignment
// -----------------------------------------------------------------------------

TEST_F(SegmentAllocatorSingleThreadTest, ReservationRoundsUpToSegments) {
	EXPECT_GE(allocator.segmentCapacity(), kTestSegments);
}

TEST(SegmentAllocatorSingleThreadStandalone, UnalignedReserveRoundsUp) {
	SegmentAllocator allocator;
	ASSERT_TRUE(allocator.init(g_kSegmentSize + 1));
	EXPECT_EQ(allocator.segmentCapacity(), 2u);
	allocator.shutdown();
}

TEST_F(SegmentAllocatorSingleThreadTest, SegmentsAreSegmentAligned) {
	void* segment = allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_NE(segment, nullptr);
	EXPECT_EQ(reinterpret_cast<std::uintptr_t>(segment) & g_kSegmentMask, 0u);
	allocator.freeSegments(segment, 1);
}

TEST_F(SegmentAllocatorSingleThreadTest, SegmentBaseMasksInteriorPointers) {
	void* segment = allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_NE(segment, nullptr);

	char* interior = static_cast<char*>(segment) + (g_kSegmentSize / 2);
	EXPECT_EQ(SegmentAllocator::segmentBase(interior), segment);

	allocator.freeSegments(segment, 1);
}

// -----------------------------------------------------------------------------
// Ownership
// -----------------------------------------------------------------------------

TEST_F(SegmentAllocatorSingleThreadTest, OwnsRejectsOutsidePointers) {
	int stackValue = 0;
	EXPECT_FALSE(allocator.owns(&stackValue));
	EXPECT_FALSE(allocator.owns(nullptr));
}

TEST_F(SegmentAllocatorSingleThreadTest, OwnsAcceptsInteriorPointers) {
	void* segment = allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_NE(segment, nullptr);

	EXPECT_TRUE(allocator.owns(segment));
	EXPECT_TRUE(allocator.owns(static_cast<char*>(segment) + g_kSegmentSize - 1));

	allocator.freeSegments(segment, 1);
}

TEST_F(SegmentAllocatorSingleThreadTest, OwnsRejectsOnePastReservationEnd) {
	std::vector<void*> held;

	// Sequential single-segment allocations on a fresh allocator land at
	// ascending indices, so the last one handed out sits at the top of the
	// reservation
	while (void* segment = allocator.allocSegments(1, SegmentKind::Small)) {
		held.push_back(segment);
	}

	ASSERT_FALSE(held.empty());

	const char* kOnePastEnd = static_cast<char*>(held.back()) + g_kSegmentSize;

	EXPECT_TRUE(allocator.owns(static_cast<char*>(held.back()) + g_kSegmentSize - 1));
	EXPECT_FALSE(allocator.owns(kOnePastEnd));

	for (void* segment : held) {
		allocator.freeSegments(segment, 1);
	}
}

// -----------------------------------------------------------------------------
// Allocation and run finding
// -----------------------------------------------------------------------------

TEST_F(SegmentAllocatorSingleThreadTest, AllocatesDistinctSegments) {
	void* first = allocator.allocSegments(1, SegmentKind::Small);
	void* second = allocator.allocSegments(1, SegmentKind::Small);

	ASSERT_NE(first, nullptr);
	ASSERT_NE(second, nullptr);
	EXPECT_NE(first, second);

	allocator.freeSegments(first, 1);
	allocator.freeSegments(second, 1);
}

TEST_F(SegmentAllocatorSingleThreadTest, MultiSegmentRunIsContiguous) {
	constexpr std::size_t kRunSegments = 4;

	void* run = allocator.allocSegments(kRunSegments, SegmentKind::HugeHead);
	ASSERT_NE(run, nullptr);

	// Segments only reserve address space, so each one needs its own commit
	// before it can be written
	const std::size_t kPagesPerSegment = g_kSegmentSize / allocator.capabilities().pageSize;
	int error = 0;

	for (std::size_t segment = 0; segment < kRunSegments; ++segment) {
		void* segmentBase = static_cast<char*>(run) + (segment * g_kSegmentSize);
		ASSERT_TRUE(allocator.ensureCommittedPages(segmentBase, kPagesPerSegment, error));
	}

	// Writing across the whole run proves the address space is one contiguous range
	std::memset(run, 0xCD, kRunSegments * g_kSegmentSize);
	EXPECT_EQ(static_cast<unsigned char*>(run)[kRunSegments * g_kSegmentSize - 1], 0xCD);

	allocator.freeSegments(run, kRunSegments);
}

TEST_F(SegmentAllocatorSingleThreadTest, ExhaustionReturnsNullptr) {
	std::vector<void*> held;

	while (void* segment = allocator.allocSegments(1, SegmentKind::Small)) {
		held.push_back(segment);
	}

	EXPECT_FALSE(held.empty());
	EXPECT_EQ(allocator.allocSegments(1, SegmentKind::Small), nullptr);

	for (void* segment : held) {
		allocator.freeSegments(segment, 1);
	}
}

TEST_F(SegmentAllocatorSingleThreadTest, RunLargerThanCapacityFails) {
	EXPECT_EQ(allocator.allocSegments(allocator.segmentCapacity() + 1, SegmentKind::HugeHead), nullptr);
}

TEST_F(SegmentAllocatorSingleThreadTest, FragmentationBlocksContiguousRun) {
	std::vector<void*> held;

	while (void* segment = allocator.allocSegments(1, SegmentKind::Small)) {
		held.push_back(segment);
	}

	ASSERT_GE(held.size(), 4u);

	// Free alternating segments, so plenty is free but nothing is adjacent
	for (std::size_t index = 0; index < held.size(); index += 2) {
		allocator.freeSegments(held[index], 1);
		held[index] = nullptr;
	}

	EXPECT_EQ(allocator.allocSegments(2, SegmentKind::HugeHead), nullptr);

	for (void* segment : held) {
		if (segment != nullptr) {
			allocator.freeSegments(segment, 1);
		}
	}
}

TEST_F(SegmentAllocatorSingleThreadTest, FreedRunIsReusable) {
	void* first = allocator.allocSegments(3, SegmentKind::HugeHead);
	ASSERT_NE(first, nullptr);
	allocator.freeSegments(first, 3);

	void* second = allocator.allocSegments(3, SegmentKind::HugeHead);
	EXPECT_EQ(second, first);
	allocator.freeSegments(second, 3);
}

TEST_F(SegmentAllocatorSingleThreadTest, AdjacentFreedSegmentsCoalesce) {
	void* first = allocator.allocSegments(1, SegmentKind::Small);
	void* second = allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_NE(first, nullptr);
	ASSERT_NE(second, nullptr);

	allocator.freeSegments(first, 1);
	allocator.freeSegments(second, 1);

	// Adjacency in the bitmap is adjacency in memory, so no work is needed for
	// two singles to satisfy a run of two
	void* run = allocator.allocSegments(2, SegmentKind::HugeHead);
	EXPECT_NE(run, nullptr);

	if (run != nullptr) {
		allocator.freeSegments(run, 2);
	}
}

// -----------------------------------------------------------------------------
// Commit prefix
// -----------------------------------------------------------------------------

TEST_F(SegmentAllocatorSingleThreadTest, FreshSegmentHasNoCommittedPages) {
	void* segment = allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_NE(segment, nullptr);

	EXPECT_EQ(allocator.committedPages(segment), 0u);

	allocator.freeSegments(segment, 1);
}

TEST_F(SegmentAllocatorSingleThreadTest, CommitGrowsPrefixAndMemoryIsWritable) {
	void* segment = allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_NE(segment, nullptr);

	int error = 0;
	ASSERT_TRUE(allocator.ensureCommittedPages(segment, 4, error));
	EXPECT_EQ(allocator.committedPages(segment), 4u);

	const std::size_t kBytes = 4 * allocator.capabilities().pageSize;
	std::memset(segment, 0xEF, kBytes);
	EXPECT_EQ(static_cast<unsigned char*>(segment)[kBytes - 1], 0xEF);

	allocator.freeSegments(segment, 1);
}

TEST_F(SegmentAllocatorSingleThreadTest, CommitIsIdempotent) {
	void* segment = allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_NE(segment, nullptr);

	int error = 0;
	ASSERT_TRUE(allocator.ensureCommittedPages(segment, 8, error));
	ASSERT_TRUE(allocator.ensureCommittedPages(segment, 8, error));
	ASSERT_TRUE(allocator.ensureCommittedPages(segment, 4, error));

	// The prefix is a target, not an increment, and it never shrinks on commit
	EXPECT_EQ(allocator.committedPages(segment), 8u);

	allocator.freeSegments(segment, 1);
}

TEST_F(SegmentAllocatorSingleThreadTest, CommitOnlyGrowsTheDelta) {
	void* segment = allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_NE(segment, nullptr);

	int error = 0;
	ASSERT_TRUE(allocator.ensureCommittedPages(segment, 4, error));
	ASSERT_TRUE(allocator.ensureCommittedPages(segment, 10, error));
	EXPECT_EQ(allocator.committedPages(segment), 10u);

	// Pages committed by the first call must still be writable after the second
	std::memset(segment, 0x11, 10 * allocator.capabilities().pageSize);

	allocator.freeSegments(segment, 1);
}

#if TRIVIAL_MEMORY_ENABLE_COMMIT_BUDGET && !TRIVIAL_MEMORY_FIXED_COMMIT_BUDGET
TEST_F(SegmentAllocatorSingleThreadTest, CommitBudgetRejectsOverBudgetCommits) {
	static bool oomFired = false;
	oomFired = false;

	const std::size_t kPageSize = allocator.capabilities().pageSize;
	allocator.setCommitBudget(2 * kPageSize);

	void* segment = allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_NE(segment, nullptr);

	int error = 0;
	ASSERT_TRUE(allocator.ensureCommittedPages(segment, 2, error));

	allocator.setOomHandler([](const OomInfo& info) {
		(void)info;
		oomFired = true;
	});

	// Growing the prefix by even one more page would exceed the budget
	EXPECT_FALSE(allocator.ensureCommittedPages(segment, 3, error));
	EXPECT_TRUE(oomFired);
	EXPECT_EQ(allocator.committedPages(segment), 2u);

	allocator.freeSegments(segment, 1);
}
#endif // TRIVIAL_MEMORY_ENABLE_COMMIT_BUDGET && !TRIVIAL_MEMORY_FIXED_COMMIT_BUDGET

TEST_F(SegmentAllocatorSingleThreadTest, CommitFillsEntireSegment) {
	void* segment = allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_NE(segment, nullptr);

	const std::size_t kPagesPerSegment = g_kSegmentSize / allocator.capabilities().pageSize;

	int error = 0;
	ASSERT_TRUE(allocator.ensureCommittedPages(segment, kPagesPerSegment, error));
	EXPECT_EQ(allocator.committedPages(segment), kPagesPerSegment);

	// The full segment must be writable right up to its last byte
	std::memset(segment, 0x77, g_kSegmentSize);
	EXPECT_EQ(static_cast<unsigned char*>(segment)[g_kSegmentSize - 1], 0x77);

	allocator.freeSegments(segment, 1);
}

TEST_F(SegmentAllocatorSingleThreadTest, CommittedPagesRejectsForeignPointers) {
	int stackValue = 0;
	EXPECT_EQ(allocator.committedPages(&stackValue), 0u);
}

// -----------------------------------------------------------------------------
// Cache and adoption
// -----------------------------------------------------------------------------

TEST_F(SegmentAllocatorSingleThreadTest, FreedSegmentKeepsCommittedPrefix) {
	void* segment = allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_NE(segment, nullptr);

	int error = 0;
	ASSERT_TRUE(allocator.ensureCommittedPages(segment, 6, error));
	allocator.freeSegments(segment, 1);

	void* adopted = allocator.allocSegments(1, SegmentKind::Medium);
	ASSERT_EQ(adopted, segment);
	EXPECT_EQ(allocator.committedPages(adopted), 6u);

	allocator.freeSegments(adopted, 1);
}

TEST_F(SegmentAllocatorSingleThreadTest, CachedSegmentIsPreferredOverFreshOne) {
	void* first = allocator.allocSegments(1, SegmentKind::Small);
	void* second = allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_NE(first, nullptr);
	ASSERT_NE(second, nullptr);

	int error = 0;
	ASSERT_TRUE(allocator.ensureCommittedPages(second, 2, error));

	// first is released with nothing committed, second with a live prefix, so
	// only second enters the cache and should win despite the lower index
	allocator.freeSegments(first, 1);
	allocator.freeSegments(second, 1);

	void* adopted = allocator.allocSegments(1, SegmentKind::Small);
	EXPECT_EQ(adopted, second);

	allocator.freeSegments(adopted, 1);
}

#if TRIVIAL_MEMORY_ENABLE_DECOMMIT
TEST_F(SegmentAllocatorSingleThreadTest, TrimShrinksPrefix) {
	void* segment = allocator.allocSegments(1, SegmentKind::ScratchArena);
	ASSERT_NE(segment, nullptr);

	int error = 0;
	ASSERT_TRUE(allocator.ensureCommittedPages(segment, 10, error));

	allocator.trimCommittedPagesTo(segment, 4);
	EXPECT_EQ(allocator.committedPages(segment), 4u);

	// The surviving prefix must still be backed
	std::memset(segment, 0x22, 4 * allocator.capabilities().pageSize);

	allocator.freeSegments(segment, 1);
}

TEST_F(SegmentAllocatorSingleThreadTest, TrimAboveCurrentPrefixDoesNothing) {
	void* segment = allocator.allocSegments(1, SegmentKind::ScratchArena);
	ASSERT_NE(segment, nullptr);

	int error = 0;
	ASSERT_TRUE(allocator.ensureCommittedPages(segment, 4, error));

	allocator.trimCommittedPagesTo(segment, 8);
	EXPECT_EQ(allocator.committedPages(segment), 4u);

	allocator.freeSegments(segment, 1);
}

TEST_F(SegmentAllocatorSingleThreadTest, RecommitAfterTrimIsWritable) {
	void* segment = allocator.allocSegments(1, SegmentKind::ScratchArena);
	ASSERT_NE(segment, nullptr);

	int error = 0;
	ASSERT_TRUE(allocator.ensureCommittedPages(segment, 8, error));
	allocator.trimCommittedPagesTo(segment, 2);
	ASSERT_TRUE(allocator.ensureCommittedPages(segment, 8, error));

	std::memset(segment, 0x33, 8 * allocator.capabilities().pageSize);
	EXPECT_EQ(allocator.committedPages(segment), 8u);

	allocator.freeSegments(segment, 1);
}
#endif // TRIVIAL_MEMORY_ENABLE_DECOMMIT

// -----------------------------------------------------------------------------
// Decay and purge
// -----------------------------------------------------------------------------

#if TRIVIAL_MEMORY_ENABLE_DECOMMIT
TEST_F(SegmentAllocatorSingleThreadTest, DecayPurgesCachedSegments) {
	void* segment = allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_NE(segment, nullptr);

	int error = 0;
	ASSERT_TRUE(allocator.ensureCommittedPages(segment, 4, error));
	allocator.freeSegments(segment, 1);

	for (std::uint32_t tick = 0; tick <= g_kDecayTicks + 1; ++tick) {
		allocator.tick();
	}

	void* adopted = allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_EQ(adopted, segment);
	EXPECT_EQ(allocator.committedPages(adopted), 0u);

	allocator.freeSegments(adopted, 1);
}

TEST_F(SegmentAllocatorSingleThreadTest, CachedSegmentSurvivesInsideDecayWindow) {
	void* segment = allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_NE(segment, nullptr);

	int error = 0;
	ASSERT_TRUE(allocator.ensureCommittedPages(segment, 4, error));
	allocator.freeSegments(segment, 1);

	allocator.tick();

	void* adopted = allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_EQ(adopted, segment);
	EXPECT_EQ(allocator.committedPages(adopted), 4u);

	allocator.freeSegments(adopted, 1);
}

TEST(SegmentAllocatorSingleThreadStandalone, CacheOverflowPurgesInsteadOfCaching) {
	constexpr std::size_t kSegments = g_kMaxCachedSegments + 2;

	SegmentAllocator allocator;
	ASSERT_TRUE(allocator.init(kSegments * g_kSegmentSize));

	std::vector<void*> held;
	held.reserve(g_kMaxCachedSegments + 1);

	for (std::size_t i = 0; i < g_kMaxCachedSegments + 1; ++i) {
		void* segment = allocator.allocSegments(1, SegmentKind::Small);
		ASSERT_NE(segment, nullptr);

		int error = 0;
		ASSERT_TRUE(allocator.ensureCommittedPages(segment, 2, error));

		held.push_back(segment);
	}

	// The cache only holds g_kMaxCachedSegments entries, so freeing one more
	// than that must purge the overflow immediately instead of caching it
	for (void* segment : held) {
		allocator.freeSegments(segment, 1);
	}

	std::vector<void*> reclaimed;
	reclaimed.reserve(g_kMaxCachedSegments);

	for (std::size_t i = 0; i < g_kMaxCachedSegments; ++i) {
		void* segment = allocator.allocSegments(1, SegmentKind::Small);
		ASSERT_NE(segment, nullptr);
		EXPECT_EQ(allocator.committedPages(segment), 2u);
		reclaimed.push_back(segment);
	}

	// Every cache slot has been drained, so this can only be the segment that
	// overflowed the cache and was purged on free
	void* overflowed = allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_NE(overflowed, nullptr);
	EXPECT_EQ(allocator.committedPages(overflowed), 0u);

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
	void* segment = allocator.allocSegments(1, SegmentKind::EcsChunks);
	ASSERT_NE(segment, nullptr);

	EXPECT_EQ(allocator.kindOf(segment), SegmentKind::EcsChunks);
	EXPECT_EQ(allocator.kindOf(static_cast<char*>(segment) + 128), SegmentKind::EcsChunks);

	allocator.freeSegments(segment, 1);
	EXPECT_EQ(allocator.kindOf(segment), SegmentKind::Invalid);
}

TEST_F(SegmentAllocatorSingleThreadTest, KindOfForeignPointerIsInvalid) {
	int stackValue = 0;
	EXPECT_EQ(allocator.kindOf(&stackValue), SegmentKind::Invalid);
}

TEST_F(SegmentAllocatorSingleThreadTest, RecordOfReflectsKindAndCommittedPages) {
	void* segment = allocator.allocSegments(1, SegmentKind::TexturePool);
	ASSERT_NE(segment, nullptr);

	int error = 0;
	ASSERT_TRUE(allocator.ensureCommittedPages(segment, 3, error));

	const SegmentRecord kRecord = allocator.recordOf(segment);
	EXPECT_EQ(kRecord.kind, SegmentKind::TexturePool);
	EXPECT_EQ(kRecord.committedPages, 3u);
	EXPECT_EQ(kRecord.runLength, 1u);

	allocator.freeSegments(segment, 1);

	int stackValue = 0;
	EXPECT_EQ(allocator.recordOf(&stackValue).kind, SegmentKind::Invalid);
}

TEST_F(SegmentAllocatorSingleThreadTest, HighWaterOnlyGrows) {
	void* first = allocator.allocSegments(2, SegmentKind::Small);
	ASSERT_NE(first, nullptr);

	const std::size_t kAfterAlloc = allocator.highWaterSegments();
	EXPECT_GE(kAfterAlloc, 2u);

	allocator.freeSegments(first, 2);
	EXPECT_EQ(allocator.highWaterSegments(), kAfterAlloc);
}
#endif // TRIVIAL_ENABLE_MEMORY_DEBUG_STATS

#if TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES
TEST_F(SegmentAllocatorSingleThreadTest, CommittedBytesTracksCommits) {
	const std::size_t kBefore = allocator.committedBytes();

	void* segment = allocator.allocSegments(1, SegmentKind::Small);
	ASSERT_NE(segment, nullptr);

	int error = 0;
	ASSERT_TRUE(allocator.ensureCommittedPages(segment, 4, error));

	const std::size_t kPageSize = allocator.capabilities().pageSize;
	EXPECT_EQ(allocator.committedBytes(), kBefore + (4 * kPageSize));

	allocator.freeSegments(segment, 1);
}
#endif // TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES

// -----------------------------------------------------------------------------
// Lifecycle
// -----------------------------------------------------------------------------

TEST(SegmentAllocatorSingleThreadStandalone, FailedInitLeavesStateClean) {
	static bool handlerFired = false;
	handlerFired = false;

	SegmentAllocator allocator;
	allocator.setOomHandler([](const OomInfo& info) {
		(void)info;
		handlerFired = true;
	});

	EXPECT_FALSE(allocator.init(SIZE_MAX / 2));
	EXPECT_TRUE(handlerFired);

	// A failed init must not leave a mapping or partial state behind
	EXPECT_TRUE(allocator.init(kTestReserve));
	EXPECT_NE(allocator.allocSegments(1, SegmentKind::Small), nullptr);

	allocator.shutdown();
}

TEST(SegmentAllocatorSingleThreadStandalone, ShutdownIsIdempotent) {
	SegmentAllocator allocator;
	ASSERT_TRUE(allocator.init(kTestReserve));

	allocator.shutdown();
	allocator.shutdown();
}

TEST(SegmentAllocatorSingleThreadStandalone, OomHandlerReportsRequestedSize) {
	static std::size_t capturedSize = 0;
	capturedSize = 0;

	SegmentAllocator allocator;
	ASSERT_TRUE(allocator.init(g_kSegmentSize));

	allocator.setOomHandler([](const OomInfo& info) {
		capturedSize = info.requestedSize;
	});

	EXPECT_EQ(allocator.allocSegments(4, SegmentKind::HugeHead), nullptr);
	EXPECT_EQ(capturedSize, 4 * g_kSegmentSize);

	allocator.shutdown();
}

} // namespace

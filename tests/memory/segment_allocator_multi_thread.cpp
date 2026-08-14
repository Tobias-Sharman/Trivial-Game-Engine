#include <atomic>
#include <cstring>
#include <span>
#include <vector>

#include <gtest/gtest.h>

#include <trivial/core/memory/segment_allocator.h>
#include <trivial/task/task_payload.h>
#include <trivial/task/task_system.h>
#include <trivial/task/task_system_config.h>

using namespace trivial::memory;

namespace {

constexpr std::size_t kConcurrentSegments = 64;
constexpr std::size_t kConcurrentReserve = kConcurrentSegments * g_kSegmentSize;
constexpr std::size_t kConcurrentTasks = 8;
constexpr std::size_t kConcurrentIterations = 256;
constexpr std::uint32_t kConcurrentWorkers = 4;

// Fixed rather than left to hardware concurrency, so a race that reproduces on
// one machine reproduces on every machine
[[nodiscard]] trivial::task::TaskSystemConfig concurrentConfig() {
	trivial::task::TaskSystemConfig config;
	config.workers.count = kConcurrentWorkers;
	config.workers.thread.name = "Segment allocator test worker";

	return config;
}

class SegmentAllocatorMultiThreadTest : public ::testing::Test {
protected:
	void SetUp() override { ASSERT_TRUE(allocator.init(kConcurrentReserve)); }

	void TearDown() override { allocator.shutdown(); }

	// Runs body on kConcurrentTasks workers and waits for all of them. Drives
	// the fixture's own TaskSystem directly rather than through the
	// setActiveTaskSystem global, since that global is shared process-wide and
	// other test files rely on it staying set to their own task system for the
	// lifetime of the binary
	template <typename Body>
	void runOnAllTasks(Body&& body) {
		std::vector<trivial::task::TaskHandle> handles;
		handles.reserve(kConcurrentTasks);

		for (std::size_t taskIndex = 0; taskIndex < kConcurrentTasks; ++taskIndex) {
			handles.push_back(taskSystem.launch(trivial::task::TaskPayload{[taskIndex, &body]() noexcept {
				body(taskIndex);
			}}));
		}

		taskSystem.wait(std::span<const trivial::task::TaskHandle>{handles});

		for (trivial::task::TaskHandle handle : handles) {
			(void)taskSystem.release(handle);
		}
	}

	SegmentAllocator allocator;
	trivial::task::TaskSystem taskSystem{concurrentConfig()};
};

// -----------------------------------------------------------------------------
// Concurrent allocation and release
// -----------------------------------------------------------------------------

// A segment handed to two threads at once would corrupt the pattern one of them
// wrote, so a mismatch here is a double handout rather than a memory error
TEST_F(SegmentAllocatorMultiThreadTest, ConcurrentAllocFreeNeverHandsOutTheSameSegment) {
	std::atomic<std::size_t> mismatches{0};
	std::atomic<std::size_t> exhaustions{0};

	runOnAllTasks([&](std::size_t taskIndex) {
		const auto kPattern = static_cast<unsigned char>(0x40 + taskIndex);

		for (std::size_t iteration = 0; iteration < kConcurrentIterations; ++iteration) {
			void* segment = allocator.allocSegments(1, SegmentKind::Small);

			if (segment == nullptr) {
				exhaustions.fetch_add(1, std::memory_order_relaxed);
				continue;
			}

			int error = 0;
			if (!allocator.ensureCommittedPages(segment, 1, error)) {
				allocator.freeSegments(segment, 1);
				continue;
			}

			const std::size_t kBytes = allocator.capabilities().pageSize;
			std::memset(segment, kPattern, kBytes);

			for (std::size_t byte = 0; byte < kBytes; ++byte) {
				if (static_cast<unsigned char*>(segment)[byte] != kPattern) {
					mismatches.fetch_add(1, std::memory_order_relaxed);
					break;
				}
			}

			allocator.freeSegments(segment, 1);
		}
	});

	EXPECT_EQ(mismatches.load(), 0u);

	// Everything must be free again, so the whole reservation is available
	std::vector<void*> held;

	while (void* segment = allocator.allocSegments(1, SegmentKind::Small)) {
		held.push_back(segment);
	}

	EXPECT_EQ(held.size(), allocator.segmentCapacity());

	for (void* segment : held) {
		allocator.freeSegments(segment, 1);
	}
}

TEST_F(SegmentAllocatorMultiThreadTest, ConcurrentMultiSegmentRunsStayContiguous) {
	std::atomic<std::size_t> mismatches{0};

	runOnAllTasks([&](std::size_t taskIndex) {
		const auto kPattern = static_cast<unsigned char>(0x80 + taskIndex);
		const std::size_t kCount = 1 + (taskIndex % 3);

		for (std::size_t iteration = 0; iteration < kConcurrentIterations / 4; ++iteration) {
			void* run = allocator.allocSegments(kCount, SegmentKind::HugeHead);

			if (run == nullptr) {
				continue;
			}

			int error = 0;
			bool committed = true;

			for (std::size_t offset = 0; offset < kCount && committed; ++offset) {
				void* segment = static_cast<char*>(run) + (offset * g_kSegmentSize);
				committed = allocator.ensureCommittedPages(segment, 1, error);
			}

			if (committed) {
				const std::size_t kPageSize = allocator.capabilities().pageSize;

				for (std::size_t offset = 0; offset < kCount; ++offset) {
					std::memset(static_cast<char*>(run) + (offset * g_kSegmentSize), kPattern, kPageSize);
				}

				for (std::size_t offset = 0; offset < kCount; ++offset) {
					const unsigned char* page = static_cast<const unsigned char*>(run) + (offset * g_kSegmentSize);

					if (page[0] != kPattern || page[kPageSize - 1] != kPattern) {
						mismatches.fetch_add(1, std::memory_order_relaxed);
						break;
					}
				}
			}

			allocator.freeSegments(run, kCount);
		}
	});

	EXPECT_EQ(mismatches.load(), 0u);
}

// -----------------------------------------------------------------------------
// Concurrent commit on segments owned by one task each
// -----------------------------------------------------------------------------

TEST_F(SegmentAllocatorMultiThreadTest, ConcurrentCommitKeepsPrefixConsistent) {
	std::atomic<std::size_t> inconsistencies{0};

	runOnAllTasks([&](std::size_t taskIndex) {
		(void)taskIndex;

		for (std::size_t iteration = 0; iteration < kConcurrentIterations / 8; ++iteration) {
			void* segment = allocator.allocSegments(1, SegmentKind::Medium);

			if (segment == nullptr) {
				continue;
			}

			int error = 0;

			for (std::size_t pages = 1; pages <= 8; ++pages) {
				if (!allocator.ensureCommittedPages(segment, pages, error)) {
					break;
				}

				if (allocator.committedPages(segment) < pages) {
					inconsistencies.fetch_add(1, std::memory_order_relaxed);
				}
			}

			allocator.freeSegments(segment, 1);
		}
	});

	EXPECT_EQ(inconsistencies.load(), 0u);
}

#if TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES
// The counter is claimed before each commit and released on every decommit and
// purge, so a drift here is a bookkeeping leak rather than a memory one
TEST_F(SegmentAllocatorMultiThreadTest, CommittedBytesReturnsToBaseline) {
	const std::size_t kBaseline = allocator.committedBytes();

	runOnAllTasks([&](std::size_t taskIndex) {
		(void)taskIndex;

		for (std::size_t iteration = 0; iteration < kConcurrentIterations / 8; ++iteration) {
			void* segment = allocator.allocSegments(1, SegmentKind::Small);

			if (segment == nullptr) {
				continue;
			}

			int error = 0;
			(void)allocator.ensureCommittedPages(segment, 4, error);
			allocator.freeSegments(segment, 1);
		}
	});

#if TRIVIAL_MEMORY_ENABLE_DECOMMIT
	// Purge everything the run left cached
	for (std::uint32_t tick = 0; tick <= g_kDecayTicks + allocator.segmentCapacity(); ++tick) {
		allocator.tick();
	}

	EXPECT_EQ(allocator.committedBytes(), kBaseline);
#else
	EXPECT_GE(allocator.committedBytes(), kBaseline);
#endif // TRIVIAL_MEMORY_ENABLE_DECOMMIT
}
#endif // TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES

} // namespace

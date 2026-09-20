#include <atomic>
#include <cstring>
#include <span>
#include <vector>

#include <gtest/gtest.h>

#include <trivial/core/memory/segment_allocator.h>
#include <trivial/core/thread/thread.h>
#include <trivial/task/task_payload.h>
#include <trivial/task/task_system.h>
#include <trivial/task/task_system_config.h>

#include "support/helpers.h"

using namespace trivial::memory;

namespace {

constexpr std::size_t g_kConcurrentSegments = 64;
constexpr std::size_t g_kConcurrentReserve = g_kConcurrentSegments * g_kSegmentSize;
constexpr std::size_t g_kConcurrentTasks = 8;
constexpr std::size_t g_kConcurrentIterations = 256;
constexpr std::uint32_t g_kConcurrentWorkers = 4;

[[nodiscard]] trivial::task::TaskSystemConfig concurrentConfig() {
	trivial::task::TaskSystemConfig config;
	config.workers.count = g_kConcurrentWorkers;

	return config;
}

class SegmentAllocatorMultiThreadTest : public ::testing::Test {
protected:
	SegmentAllocatorMultiThreadTest() {
		m_mainThread.adoptCurrentThread({.name = "Test Main", .type = trivial::thread::ThreadType::Main});
	}

	void SetUp() override { ASSERT_TRUE(m_allocator.init(g_kConcurrentReserve)); }

	void TearDown() override { m_allocator.shutdown(); }

	template <typename Body>
	void runOnAllTasks(Body body) {
		std::vector<trivial::task::TaskHandle> handles;
		handles.reserve(g_kConcurrentTasks);

		for (std::size_t taskIndex = 0; taskIndex < g_kConcurrentTasks; ++taskIndex) {
			handles.push_back(m_taskSystem.launch(trivial::task::TaskPayload{
			    [taskIndex, &body]() noexcept {
				    body(taskIndex);
			    },
			}));
		}

		m_taskSystem.wait(std::span<const trivial::task::TaskHandle>{handles});

		for (trivial::task::TaskHandle handle : handles) {
			(void)m_taskSystem.release(handle);
		}
	}

	SegmentAllocator m_allocator;
	trivial::task::TaskSystemConfig m_config = concurrentConfig();
	trivial::thread::Thread m_mainThread;
	trivial::tests::ScopedParkingLot m_parkingLotScope{m_config.workers.count + m_config.workers.maxStandbyWorkers};
	trivial::task::TaskSystem m_taskSystem{m_config};
};

// -----------------------------------------------------------------------------
// Concurrent allocation and release
// -----------------------------------------------------------------------------

TEST_F(SegmentAllocatorMultiThreadTest, AllocFreeNeverHandsOutSameSegment) {
	std::atomic<std::size_t> mismatches{0};
	std::atomic<std::size_t> exhaustions{0};

	runOnAllTasks([&](std::size_t taskIndex) {
		const auto kPattern = static_cast<unsigned char>(0x40 + taskIndex);

		for (std::size_t iteration = 0; iteration < g_kConcurrentIterations; ++iteration) {
			void* segment = m_allocator.allocSegments(1, SegmentKind::Small);

			if (segment == nullptr) {
				exhaustions.fetch_add(1, std::memory_order_relaxed);
				continue;
			}

			int error = 0;
			if (!m_allocator.ensureCommittedPages(segment, 1, error)) {
				m_allocator.freeSegments(segment, 1);
				continue;
			}

			const std::size_t kBytes = m_allocator.capabilities().pageSize;
			std::memset(segment, kPattern, kBytes);

			for (std::size_t byte = 0; byte < kBytes; ++byte) {
				if (static_cast<unsigned char*>(segment)[byte] != kPattern) {
					mismatches.fetch_add(1, std::memory_order_relaxed);
					break;
				}
			}

			m_allocator.freeSegments(segment, 1);
		}
	});

	EXPECT_EQ(mismatches.load(), 0u);

	// Everything must be free again, so the whole reservation is available
	std::vector<void*> held;

	while (void* segment = m_allocator.allocSegments(1, SegmentKind::Small)) {
		held.push_back(segment);
	}

	EXPECT_EQ(held.size(), m_allocator.segmentCapacity());

	for (void* segment : held) {
		m_allocator.freeSegments(segment, 1);
	}
}

TEST_F(SegmentAllocatorMultiThreadTest, MultiSegmentRunsStayContiguous) {
	std::atomic<std::size_t> mismatches{0};

	runOnAllTasks([&](std::size_t taskIndex) {
		const auto kPattern = static_cast<unsigned char>(0x80 + taskIndex);
		const std::size_t kCount = 1 + (taskIndex % 3);

		for (std::size_t iteration = 0; iteration < g_kConcurrentIterations / 4; ++iteration) {
			void* run = m_allocator.allocSegments(kCount, SegmentKind::HugeHead);

			if (run == nullptr) {
				continue;
			}

			int error = 0;
			bool committed = true;

			for (std::size_t offset = 0; offset < kCount && committed; ++offset) {
				void* segment = static_cast<char*>(run) + (offset * g_kSegmentSize);
				committed = m_allocator.ensureCommittedPages(segment, 1, error);
			}

			if (committed) {
				const std::size_t kPageSize = m_allocator.capabilities().pageSize;

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

			m_allocator.freeSegments(run, kCount);
		}
	});

	EXPECT_EQ(mismatches.load(), 0u);
}

// -----------------------------------------------------------------------------
// Concurrent commit on segments owned by one task each
// -----------------------------------------------------------------------------

TEST_F(SegmentAllocatorMultiThreadTest, CommitKeepsPrefixConsistent) {
	std::atomic<std::size_t> inconsistencies{0};

	runOnAllTasks([&](std::size_t taskIndex) {
		(void)taskIndex;

		for (std::size_t iteration = 0; iteration < g_kConcurrentIterations / 8; ++iteration) {
			void* segment = m_allocator.allocSegments(1, SegmentKind::Medium);

			if (segment == nullptr) {
				continue;
			}

			int error = 0;

			for (std::size_t pages = 1; pages <= 8; ++pages) {
				if (!m_allocator.ensureCommittedPages(segment, pages, error)) {
					break;
				}

				if (m_allocator.committedPages(segment) < pages) {
					inconsistencies.fetch_add(1, std::memory_order_relaxed);
				}
			}

			m_allocator.freeSegments(segment, 1);
		}
	});

	EXPECT_EQ(inconsistencies.load(), 0u);
}

#if TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES
// The counter is claimed before each commit and released on every decommit and
// purge, so a drift here is a bookkeeping leak rather than a memory one
TEST_F(SegmentAllocatorMultiThreadTest, CommittedBytesReturnsToBaseline) {
	const std::size_t kBaseline = m_allocator.committedBytes();

	runOnAllTasks([&](std::size_t taskIndex) {
		(void)taskIndex;

		for (std::size_t iteration = 0; iteration < g_kConcurrentIterations / 8; ++iteration) {
			void* segment = m_allocator.allocSegments(1, SegmentKind::Small);

			if (segment == nullptr) {
				continue;
			}

			int error = 0;
			(void)m_allocator.ensureCommittedPages(segment, 4, error);
			m_allocator.freeSegments(segment, 1);
		}
	});

#if TRIVIAL_MEMORY_ENABLE_DECOMMIT
	// Purge everything the run left cached
	for (std::uint32_t tick = 0; tick <= g_kDecayTicks + m_allocator.segmentCapacity(); ++tick) {
		m_allocator.tick();
	}

	EXPECT_EQ(m_allocator.committedBytes(), kBaseline);
#else
	EXPECT_GE(allocator.committedBytes(), kBaseline);
#endif // TRIVIAL_MEMORY_ENABLE_DECOMMIT
}
#endif // TRIVIAL_MEMORY_TRACK_COMMITTED_BYTES

} // namespace

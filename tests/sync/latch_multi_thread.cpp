#include <atomic>
#include <cstddef>

#include <gtest/gtest.h>

#include <trivial/core/sync/latch.h>
#include <trivial/core/thread/thread.h>

#include "core/heap_array.h"
#include "support/helpers.h"

namespace {

constexpr std::size_t g_kWorkerThreads = 8;

struct LatchCountDownContext {
	trivial::sync::Latch* latch;
	std::atomic<std::size_t>* completed;
};

void countDownWorker(void* arg) {
	auto* context = static_cast<LatchCountDownContext*>(arg);
	context->completed->fetch_add(1, std::memory_order_acq_rel);
	context->latch->countDown();
}

TEST(LatchMultiThreadTest, CountDownsNeverRace) {
	trivial::sync::Latch latch(g_kWorkerThreads);
	std::atomic<std::size_t> completed{0};
	LatchCountDownContext context{.latch = &latch, .completed = &completed};

	trivial::tests::runOnAllThreads(g_kWorkerThreads, &countDownWorker, &context);

	EXPECT_EQ(completed.load(std::memory_order_acquire), g_kWorkerThreads);
	EXPECT_EQ(latch.remaining(), 0U);

	latch.wait(); // already at zero - must return immediately
}

TEST(LatchMultiThreadTest, WaitUnblocksAfterCountDown) {
	trivial::tests::ScopedParkingLot parkingLotScope(g_kWorkerThreads);

	trivial::thread::Thread mainThread;
	mainThread.adoptCurrentThread({.name = "main-test-thread", .type = trivial::thread::ThreadType::Main});

	trivial::sync::Latch latch(g_kWorkerThreads);
	std::atomic<std::size_t> completed{0};
	LatchCountDownContext context{.latch = &latch, .completed = &completed};

	trivial::core::HeapArray<trivial::thread::Thread> threads(g_kWorkerThreads);
	for (std::size_t i = 0; i < g_kWorkerThreads; ++i) {
		trivial::thread::ThreadConfig config;
#if TRIVIAL_PLATFORM_POSIX
		trivial::tests::attachStackAllocator(config);
#endif // TRIVIAL_PLATFORM_POSIX

		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
		trivial::thread::ThreadCreateResult result = threads[i].create(config, &countDownWorker, &context);
		ASSERT_EQ(result.error, trivial::thread::ThreadCreateError::None);
	}

	latch.wait();

	EXPECT_EQ(completed.load(std::memory_order_acquire), g_kWorkerThreads);
	EXPECT_EQ(latch.remaining(), 0U);

	for (std::size_t i = 0; i < g_kWorkerThreads; ++i) {
		threads[i].join(); // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
	}
}

} // namespace

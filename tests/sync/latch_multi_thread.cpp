#include <atomic>
#include <cstddef>

#include <gtest/gtest.h>

#include <trivial/core/platform.h>
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

void countDownWorker(void* arg) noexcept {
	const LatchCountDownContext* context = static_cast<LatchCountDownContext*>(arg);
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

	latch.wait();
}

TEST(LatchMultiThreadTest, WaitUnblocksAfterCountDown) {
	const trivial::tests::ScopedParkingLot kParkingLotScope(g_kWorkerThreads);

	trivial::thread::Thread mainThread;
	mainThread.adoptCurrentThread({.name = "main-test-thread", .type = trivial::thread::ThreadType::Main});

	trivial::sync::Latch latch(g_kWorkerThreads);
	std::atomic<std::size_t> completed{0};
	LatchCountDownContext context{.latch = &latch, .completed = &completed};

	trivial::core::HeapArray<trivial::thread::Thread> threads(g_kWorkerThreads);
	for (std::size_t i = 0; i < g_kWorkerThreads; ++i) {
#if TRIVIAL_PLATFORM_POSIX
		trivial::thread::ThreadConfig config;
		trivial::tests::attachStackAllocator(config);

		const trivial::thread::ThreadCreateResult kResult = threads[i].create(config, &countDownWorker, &context);
		ASSERT_EQ(kResult.error, trivial::thread::ThreadCreateError::None);
#else
		const trivial::thread::ThreadConfig kConfig{};

		const trivial::thread::ThreadCreateResult kResult = threads[i].create(kConfig, &countDownWorker, &context);
		ASSERT_EQ(kResult.error, trivial::thread::ThreadCreateError::None);
#endif // TRIVIAL_PLATFORM_POSIX
	}

	latch.wait();

	EXPECT_EQ(completed.load(std::memory_order_acquire), g_kWorkerThreads);
	EXPECT_EQ(latch.remaining(), 0U);

	for (std::size_t i = 0; i < g_kWorkerThreads; ++i) {
		threads[i].join();
	}
}

} // namespace

#include <atomic>
#include <cstddef>

#include <gtest/gtest.h>

#include <trivial/core/sync/semaphore.h>
#include <trivial/core/thread/thread.h>

#include "support/helpers.h"

namespace {

constexpr std::size_t g_kConcurrentThreads = 8;
constexpr std::size_t g_kConcurrentIterations = 10000;

struct SemaphoreCounterContext {
	trivial::sync::Semaphore* semaphore;
	std::size_t* counter;
};

void incrementWorker(void* arg) {
	auto* context = static_cast<SemaphoreCounterContext*>(arg);

	for (std::size_t i = 0; i < g_kConcurrentIterations; ++i) {
		context->semaphore->acquire();
		++(*context->counter);
		context->semaphore->release();
	}
}

TEST(SemaphoreMultiThreadTest, AcquireReleaseNeverRace) {
	trivial::sync::Semaphore semaphore(1);
	std::size_t counter = 0;
	SemaphoreCounterContext context{.semaphore = &semaphore, .counter = &counter};

	trivial::tests::runOnAllThreads(g_kConcurrentThreads, &incrementWorker, &context);

	EXPECT_EQ(counter, g_kConcurrentThreads * g_kConcurrentIterations);
}

struct SemaphoreReleaseContext {
	trivial::sync::Semaphore* semaphore;
	std::atomic<bool>* released;
};

void releaseWorker(void* arg) {
	auto* context = static_cast<SemaphoreReleaseContext*>(arg);
	context->released->store(true, std::memory_order_release);
	context->semaphore->release();
}

TEST(SemaphoreMultiThreadTest, AcquireUnblocksAfterRelease) {
	trivial::tests::ScopedParkingLot parkingLotScope(1);

	trivial::thread::Thread mainThread;
	mainThread.adoptCurrentThread({.name = "main-test-thread", .type = trivial::thread::ThreadType::Main});

	trivial::sync::Semaphore semaphore(0);
	std::atomic<bool> released{false};
	SemaphoreReleaseContext context{.semaphore = &semaphore, .released = &released};

	trivial::thread::Thread releaser;
	trivial::thread::ThreadConfig config;
#if TRIVIAL_PLATFORM_POSIX
	trivial::tests::attachStackAllocator(config);
#endif // TRIVIAL_PLATFORM_POSIX

	trivial::thread::ThreadCreateResult result = releaser.create(config, &releaseWorker, &context);
	ASSERT_EQ(result.error, trivial::thread::ThreadCreateError::None);

	semaphore.acquire();

	EXPECT_TRUE(released.load(std::memory_order_acquire));

	releaser.join();
}

} // namespace

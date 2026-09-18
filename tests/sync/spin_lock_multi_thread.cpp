#include <cstddef>

#include <gtest/gtest.h>

#include <trivial/core/sync/spin_lock.h>

#include <support/helpers.h>

namespace {

constexpr std::size_t g_kConcurrentThreads = 8;
constexpr std::size_t g_kConcurrentIterations = 10000;

struct LockCounterContext {
	trivial::sync::SpinLock* lock;
	std::size_t* counter;
};

void lockWorker(void* arg) {
	auto* context = static_cast<LockCounterContext*>(arg);

	for (std::size_t i = 0; i < g_kConcurrentIterations; ++i) {
		context->lock->lock();
		++(*context->counter);
		context->lock->unlock();
	}
}

void tryLockWorker(void* arg) {
	auto* context = static_cast<LockCounterContext*>(arg);

	for (std::size_t i = 0; i < g_kConcurrentIterations; ++i) {
		while (!context->lock->tryLock()) {}

		++(*context->counter);
		context->lock->unlock();
	}
}

TEST(SpinLockMultiThreadTest, LockIncrementsNeverRace) {
	trivial::sync::SpinLock lock;
	std::size_t counter = 0;
	LockCounterContext context{.lock = &lock, .counter = &counter};

	trivial::tests::runOnAllThreads(g_kConcurrentThreads, &lockWorker, &context);

	EXPECT_EQ(counter, g_kConcurrentThreads * g_kConcurrentIterations);
}

TEST(SpinLockMultiThreadTest, TryLockIncrementsNeverRace) {
	trivial::sync::SpinLock lock;
	std::size_t counter = 0;
	LockCounterContext context{.lock = &lock, .counter = &counter};

	trivial::tests::runOnAllThreads(g_kConcurrentThreads, &tryLockWorker, &context);

	EXPECT_EQ(counter, g_kConcurrentThreads * g_kConcurrentIterations);
}

} // namespace

#include <atomic>
#include <cstddef>

#include <gtest/gtest.h>

#include <trivial/core/sync/condition_variable.h>
#include <trivial/core/sync/mutex.h>
#include <trivial/core/thread/thread.h>

#include "core/heap_array.h"
#include "support/helpers.h"

namespace {

constexpr std::size_t g_kWaiterThreads = 8;

struct WaitContext {
	trivial::sync::Mutex* mutex;
	trivial::sync::ConditionVariable* conditionVariable;
	bool* ready;
	std::atomic<std::size_t>* wokeCount;
};

void waitWorker(void* arg) {
	auto* context = static_cast<WaitContext*>(arg);

	context->mutex->lock();
	while (!*context->ready) {
		context->conditionVariable->wait(*context->mutex);
	}
	context->mutex->unlock();

	context->wokeCount->fetch_add(1, std::memory_order_acq_rel);
}

TEST(ConditionVariableMultiThreadTest, WaitUnblocksOnNotifyOne) {
	trivial::tests::ScopedParkingLot parkingLotScope(1);

	trivial::sync::Mutex mutex;
	trivial::sync::ConditionVariable conditionVariable;
	bool ready = false;
	std::atomic<std::size_t> wokeCount{0};
	WaitContext context{
	    .mutex = &mutex,
	    .conditionVariable = &conditionVariable,
	    .ready = &ready,
	    .wokeCount = &wokeCount,
	};

	trivial::thread::Thread waiter;
	trivial::thread::ThreadConfig config;
#if TRIVIAL_PLATFORM_POSIX
	trivial::tests::attachStackAllocator(config);
#endif // TRIVIAL_PLATFORM_POSIX

	trivial::thread::ThreadCreateResult result = waiter.create(config, &waitWorker, &context);
	ASSERT_EQ(result.error, trivial::thread::ThreadCreateError::None);

	mutex.lock();
	ready = true;
	mutex.unlock();
	conditionVariable.notifyOne(mutex);

	waiter.join();

	EXPECT_EQ(wokeCount.load(std::memory_order_acquire), 1U);
}

TEST(ConditionVariableMultiThreadTest, NotifyAllWakesEveryWaiter) {
	trivial::tests::ScopedParkingLot parkingLotScope(g_kWaiterThreads);

	trivial::sync::Mutex mutex;
	trivial::sync::ConditionVariable conditionVariable;
	bool ready = false;
	std::atomic<std::size_t> wokeCount{0};
	WaitContext context{
	    .mutex = &mutex,
	    .conditionVariable = &conditionVariable,
	    .ready = &ready,
	    .wokeCount = &wokeCount,
	};

	trivial::core::HeapArray<trivial::thread::Thread> waiters(g_kWaiterThreads);
	for (std::size_t i = 0; i < g_kWaiterThreads; ++i) {
		trivial::thread::ThreadConfig config;
#if TRIVIAL_PLATFORM_POSIX
		trivial::tests::attachStackAllocator(config);
#endif // TRIVIAL_PLATFORM_POSIX

		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
		trivial::thread::ThreadCreateResult result = waiters[i].create(config, &waitWorker, &context);
		ASSERT_EQ(result.error, trivial::thread::ThreadCreateError::None);
	}

	mutex.lock();
	ready = true;
	mutex.unlock();
	conditionVariable.notifyAll(mutex);

	for (std::size_t i = 0; i < g_kWaiterThreads; ++i) {
		waiters[i].join(); // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
	}

	EXPECT_EQ(wokeCount.load(std::memory_order_acquire), g_kWaiterThreads);
}

} // namespace

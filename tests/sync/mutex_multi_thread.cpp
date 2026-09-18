#include <cstddef>
#include <support/helpers.h>

#include <gtest/gtest.h>

#include <trivial/core/sync/mutex.h>

namespace {

constexpr std::size_t g_kConcurrentThreads = 8;
constexpr std::size_t g_kConcurrentIterations = 10000;

struct MutexCounterContext {
	trivial::sync::Mutex* mutex;
	std::size_t* counter;
};

void incrementWorker(void* arg) {
	auto* context = static_cast<MutexCounterContext*>(arg);

	for (std::size_t i = 0; i < g_kConcurrentIterations; ++i) {
		context->mutex->lock();
		++(*context->counter);
		context->mutex->unlock();
	}
}

TEST(MutexMultiThreadTest, LockIncrementsNeverRace) {
	trivial::sync::Mutex mutex;
	std::size_t counter = 0;
	MutexCounterContext context{.mutex = &mutex, .counter = &counter};

	trivial::tests::runOnAllThreads(g_kConcurrentThreads, &incrementWorker, &context);

	EXPECT_EQ(counter, g_kConcurrentThreads * g_kConcurrentIterations);
}

} // namespace

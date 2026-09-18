#include <gtest/gtest.h>

#include <trivial/core/sync/semaphore.h>

namespace {

TEST(SemaphoreSingleThreadTest, TryAcquireSucceedsUpToInitialCount) {
	trivial::sync::Semaphore semaphore(2);

	EXPECT_TRUE(semaphore.tryAcquire());
	EXPECT_TRUE(semaphore.tryAcquire());
	EXPECT_FALSE(semaphore.tryAcquire());
}

TEST(SemaphoreSingleThreadTest, TryAcquireFailsWhenZero) {
	trivial::sync::Semaphore semaphore(0);

	EXPECT_FALSE(semaphore.tryAcquire());
}

} // namespace

#include <gtest/gtest.h>

#include <trivial/core/sync/latch.h>

namespace {

TEST(LatchSingleThreadTest, RemainingMatchesInitialCount) {
	const trivial::sync::Latch kLatch(3);
	EXPECT_EQ(kLatch.remaining(), 3U);
}

TEST(LatchSingleThreadTest, CountDownDecrementsRemaining) {
	trivial::sync::Latch latch(3);

	latch.countDown();
	EXPECT_EQ(latch.remaining(), 2U);

	latch.countDown();
	EXPECT_EQ(latch.remaining(), 1U);
}

} // namespace

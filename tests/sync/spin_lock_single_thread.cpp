#include <gtest/gtest.h>

#include <trivial/core/sync/spin_lock.h>

namespace {

TEST(SpinLockSingleThreadTest, DefaultConstructedLockIsUnlocked) {
	trivial::sync::SpinLock lock;
	EXPECT_TRUE(lock.tryLock());
	lock.unlock();
}

TEST(SpinLockSingleThreadTest, TryLockFailsWhileHeld) {
	trivial::sync::SpinLock lock;
	ASSERT_TRUE(lock.tryLock());
	EXPECT_FALSE(lock.tryLock());
	lock.unlock();
}

TEST(SpinLockSingleThreadTest, UnlockAllowsReacquisition) {
	trivial::sync::SpinLock lock;
	ASSERT_TRUE(lock.tryLock());
	lock.unlock();
	EXPECT_TRUE(lock.tryLock());
	lock.unlock();
}

TEST(SpinLockSingleThreadTest, RepeatedLockUnlockCyclesStayConsistent) {
	trivial::sync::SpinLock lock;

	for (int i = 0; i < 100; ++i) { // NOLINT(readability-magic-numbers)
		lock.lock();
		EXPECT_FALSE(lock.tryLock());
		lock.unlock();
	}
}

} // namespace

#include <gtest/gtest.h>

#include <trivial/core/sync/mutex.h>

namespace {

TEST(MutexSingleThreadTest, LockUnlockDoesNotHang) {
	trivial::sync::Mutex mutex;
	mutex.lock();
	mutex.unlock();
}

TEST(MutexSingleThreadTest, CyclesStayConsistent) {
	trivial::sync::Mutex mutex;

	for (int i = 0; i < 100; ++i) { // NOLINT(readability-magic-numbers)
		mutex.lock();
		mutex.unlock();
	}
}

} // namespace

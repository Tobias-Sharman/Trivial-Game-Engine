#include <gtest/gtest.h>

#include <trivial/core/sync/condition_variable.h>
#include <trivial/core/sync/mutex.h>

#include "support/helpers.h"

namespace {

TEST(ConditionVariableSingleThreadTest, NotifyWithNoWaitersIsSafe) {
	trivial::tests::ScopedParkingLot parkingLotScope(0);

	trivial::sync::Mutex mutex;
	trivial::sync::ConditionVariable conditionVariable;

	conditionVariable.notifyOne(mutex);
	conditionVariable.notifyAll(mutex);

	mutex.lock();
	mutex.unlock();
}

} // namespace

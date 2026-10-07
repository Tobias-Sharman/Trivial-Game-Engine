#include <cstdint>
#include <limits>

#include <gtest/gtest.h>

#include <trivial/core/sync/event.h>
#include <trivial/core/thread/thread.h>
#include <trivial/core/time/duration.h>

#include "support/helpers.h"

namespace {

TEST(EventSingleThreadTest, IsTriggeredDefaultsFalse) {
	const trivial::sync::Event kEvent;
	EXPECT_FALSE(kEvent.isTriggered());
}

TEST(EventSingleThreadTest, ResetIsSafeWhenNotTriggered) {
	trivial::sync::Event event;
	event.reset();
	EXPECT_FALSE(event.isTriggered());
}

TEST(EventSingleThreadTest, WaitForMaximumTimeoutReturnsWhenTriggered) {
	const trivial::tests::ScopedParkingLot kParkingLotScope(0);

	trivial::sync::Event event;
	event.trigger();

	EXPECT_TRUE(event.waitFor(trivial::time::nanoseconds(std::numeric_limits<std::int64_t>::max())));
}

TEST(EventSingleThreadTest, WaitForTimesOutThenSucceedsAfterTrigger) {
	const trivial::tests::ScopedParkingLot kParkingLotScope(0);

	trivial::thread::Thread mainThread;
	mainThread.adoptCurrentThread({.name = "main-test-thread", .type = trivial::thread::ThreadType::Main});

	trivial::sync::Event event;

	const bool kTriggeredBeforeTimeout = event.waitFor(trivial::time::milliseconds(20));

	EXPECT_FALSE(kTriggeredBeforeTimeout);
	EXPECT_FALSE(event.isTriggered());

	event.trigger();

	EXPECT_TRUE(event.isTriggered());
	EXPECT_TRUE(event.waitFor(trivial::time::milliseconds(20)));
}

} // namespace

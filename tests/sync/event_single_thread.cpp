#include <chrono>

#include <gtest/gtest.h>

#include <trivial/core/sync/event.h>
#include <trivial/core/thread/thread.h>

#include "support/helpers.h"

namespace {

TEST(EventSingleThreadTest, IsTriggeredDefaultsFalse) {
	trivial::sync::Event event;
	EXPECT_FALSE(event.isTriggered());
}

TEST(EventSingleThreadTest, ResetIsSafeWhenNotTriggered) {
	trivial::sync::Event event;
	event.reset();
	EXPECT_FALSE(event.isTriggered());
}

TEST(EventSingleThreadTest, WaitForTimesOutThenSucceedsAfterTrigger) {
	trivial::tests::ScopedParkingLot parkingLotScope(0);

	trivial::thread::Thread mainThread;
	mainThread.adoptCurrentThread({.name = "main-test-thread", .type = trivial::thread::ThreadType::Main});

	trivial::sync::Event event;

	const bool kTriggeredBeforeTimeout = event.waitFor(std::chrono::milliseconds(20));

	EXPECT_FALSE(kTriggeredBeforeTimeout);
	EXPECT_FALSE(event.isTriggered());

	event.trigger();

	EXPECT_TRUE(event.isTriggered());
	EXPECT_TRUE(event.waitFor(std::chrono::milliseconds(20)));
}

} // namespace

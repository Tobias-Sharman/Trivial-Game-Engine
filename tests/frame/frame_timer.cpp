#include <trivial/frame/frame_timer.h>

#include <gtest/gtest.h>

#include <trivial/core/time/duration.h>
#include <trivial/core/time/time.h>
#include <trivial/frame/frame_config.h>

namespace trivial {

namespace {

TEST(FrameTimerTest, ResetClearsDeltas) {
	FrameTimer timer;
	timer.reset();

	EXPECT_EQ(timer.delta(), time::Duration{});
	EXPECT_EQ(timer.rawDelta(), time::Duration{});
	EXPECT_EQ(timer.elapsed(), time::Duration{});
	EXPECT_DOUBLE_EQ(timer.deltaSeconds(), 0.0);
}

TEST(FrameTimerTest, TickMeasuresElapsedTime) {
	FrameTimer timer;
	timer.reset();

	time::sleepFor(time::milliseconds(5));
	timer.tick();

	EXPECT_GE(timer.rawDelta(), time::milliseconds(5));
	EXPECT_EQ(timer.delta(), timer.rawDelta());
	EXPECT_DOUBLE_EQ(timer.deltaSeconds(), timer.delta().toSecondsDouble());
}

TEST(FrameTimerTest, TickClampsLongFrames) {
	FrameTimer timer;
	timer.reset();

	time::sleepFor(time::milliseconds(TRIVIAL_FRAME_MAX_DELTA_MILLISECONDS + 50));
	timer.tick();

	EXPECT_GT(timer.rawDelta(), time::milliseconds(TRIVIAL_FRAME_MAX_DELTA_MILLISECONDS));
	EXPECT_EQ(timer.delta(), time::milliseconds(TRIVIAL_FRAME_MAX_DELTA_MILLISECONDS));
}

TEST(FrameTimerTest, TickMeasuresFromPreviousTick) {
	FrameTimer timer;
	timer.reset();

	time::sleepFor(time::milliseconds(20));
	timer.tick();

	timer.tick();

	EXPECT_LT(timer.rawDelta(), time::milliseconds(20));
}

TEST(FrameTimerTest, ElapsedSumsRawDeltasSinceReset) {
	FrameTimer timer;
	timer.reset();

	time::sleepFor(time::milliseconds(5));
	timer.tick();
	const time::Duration kFirstDelta = timer.rawDelta();

	EXPECT_EQ(timer.elapsed(), kFirstDelta);

	time::sleepFor(time::milliseconds(5));
	timer.tick();

	EXPECT_EQ(timer.elapsed(), kFirstDelta + timer.rawDelta());
	EXPECT_DOUBLE_EQ(timer.elapsedSeconds(), timer.elapsed().toSecondsDouble());

	timer.reset();

	EXPECT_EQ(timer.elapsed(), time::Duration{});
}

} // namespace

} // namespace trivial

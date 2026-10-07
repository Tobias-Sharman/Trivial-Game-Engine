#include <trivial/world/world_time.h>

#include <cstdint>

#include <gtest/gtest.h>

#include <trivial/core/time/duration.h>

namespace trivial::world {

namespace {

TEST(WorldTimeTest, DefaultsToZeroAndUnscaled) {
	const WorldTime kWorldTime;

	EXPECT_EQ(kWorldTime.realElapsed(), time::Duration{});
	EXPECT_EQ(kWorldTime.unpausedElapsed(), time::Duration{});
	EXPECT_EQ(kWorldTime.gameElapsed(), time::Duration{});
	EXPECT_DOUBLE_EQ(kWorldTime.timeScale(), 1.0);
	EXPECT_DOUBLE_EQ(kWorldTime.pauseMultiplier(), 1.0);
}

TEST(WorldTimeTest, AccumulatesRealTimeExactly) {
	WorldTime worldTime;

	for (std::int32_t i = 0; i < 1'000; ++i) {
		worldTime.tick(time::nanoseconds(16'666'667));
	}

	EXPECT_EQ(worldTime.realDelta(), time::nanoseconds(16'666'667));
	EXPECT_EQ(worldTime.realElapsed(), time::nanoseconds(16'666'667'000));
	EXPECT_EQ(worldTime.unpausedElapsed(), worldTime.realElapsed());
	EXPECT_EQ(worldTime.gameElapsed(), worldTime.realElapsed());
}

TEST(WorldTimeTest, TimeScaleAppliesToUnpausedAndGameTime) {
	WorldTime worldTime;
	worldTime.setTimeScale(0.5);

	worldTime.tick(time::milliseconds(10));

	EXPECT_EQ(worldTime.realDelta(), time::milliseconds(10));
	EXPECT_EQ(worldTime.unpausedDelta(), time::milliseconds(5));
	EXPECT_EQ(worldTime.gameDelta(), time::milliseconds(5));
	EXPECT_DOUBLE_EQ(worldTime.gameDeltaSeconds(), 0.005);
}

TEST(WorldTimeTest, ScaledTimeCarriesFractionalNanoseconds) {
	WorldTime worldTime;
	worldTime.setTimeScale(1.0 / 3.0);

	for (std::int32_t i = 0; i < 3'000; ++i) {
		worldTime.tick(time::nanoseconds(1));
	}

	EXPECT_GE(worldTime.unpausedElapsed(), time::nanoseconds(999));
	EXPECT_LE(worldTime.unpausedElapsed(), time::nanoseconds(1'000));
}

TEST(WorldTimeTest, ZeroPauseMultiplierStopsOnlyGameTime) {
	WorldTime worldTime;
	worldTime.tick(time::milliseconds(10));

	worldTime.setPauseMultiplier(0.0);
	worldTime.tick(time::milliseconds(10));

	EXPECT_EQ(worldTime.gameDelta(), time::Duration{});
	EXPECT_EQ(worldTime.gameElapsed(), time::milliseconds(10));
	EXPECT_EQ(worldTime.unpausedElapsed(), time::milliseconds(20));
	EXPECT_EQ(worldTime.realElapsed(), time::milliseconds(20));

	worldTime.setPauseMultiplier(1.0);
	worldTime.tick(time::milliseconds(10));

	EXPECT_EQ(worldTime.gameElapsed(), time::milliseconds(20));
}

TEST(WorldTimeTest, PartialPauseMultiplierSlowsGameTime) {
	WorldTime worldTime;
	worldTime.setTimeScale(0.5);
	worldTime.setPauseMultiplier(0.1);

	worldTime.tick(time::milliseconds(100));

	EXPECT_EQ(worldTime.unpausedDelta(), time::milliseconds(50));
	EXPECT_EQ(worldTime.gameDelta(), time::milliseconds(5));
}

TEST(WorldTimeTest, NegativePauseMultiplierRewindsGameTime) {
	WorldTime worldTime;
	worldTime.tick(time::milliseconds(30));

	worldTime.setPauseMultiplier(-1.0);
	worldTime.tick(time::milliseconds(10));

	EXPECT_EQ(worldTime.gameDelta(), time::milliseconds(-10));
	EXPECT_EQ(worldTime.gameElapsed(), time::milliseconds(20));
	EXPECT_EQ(worldTime.unpausedElapsed(), time::milliseconds(40));
}

TEST(WorldTimeTest, NegativeTimeScaleRewindsUnpausedAndGameTime) {
	WorldTime worldTime;
	worldTime.setTimeScale(-2.0);

	worldTime.tick(time::milliseconds(10));

	EXPECT_DOUBLE_EQ(worldTime.timeScale(), -2.0);
	EXPECT_EQ(worldTime.unpausedDelta(), time::milliseconds(-20));
	EXPECT_EQ(worldTime.gameDelta(), time::milliseconds(-20));
	EXPECT_EQ(worldTime.realDelta(), time::milliseconds(10));
}

TEST(WorldTimeTest, ResetClearsTimeButKeepsSettings) {
	WorldTime worldTime;
	worldTime.setTimeScale(2.0);
	worldTime.setPauseMultiplier(0.5);
	worldTime.tick(time::milliseconds(10));

	worldTime.reset();

	EXPECT_EQ(worldTime.realElapsed(), time::Duration{});
	EXPECT_EQ(worldTime.unpausedElapsed(), time::Duration{});
	EXPECT_EQ(worldTime.gameElapsed(), time::Duration{});
	EXPECT_DOUBLE_EQ(worldTime.timeScale(), 2.0);
	EXPECT_DOUBLE_EQ(worldTime.pauseMultiplier(), 0.5);
}

} // namespace

} // namespace trivial::world

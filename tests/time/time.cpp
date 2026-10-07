#include <trivial/core/time/time.h>

#include <chrono>
#include <cstdint>

#include <gtest/gtest.h>

#include <trivial/core/time/duration.h>
#include <trivial/core/time/instant.h>

namespace trivial::time {

namespace {

TEST(TimeTest, ScaleTicksMultipliesExactFrequencies) {
	EXPECT_EQ(scaleTicks(0, 100, 1), 0U);
	EXPECT_EQ(scaleTicks(10'000'000, 100, 1), 1'000'000'000U);
	EXPECT_EQ(scaleTicks(1, 1, 1), 1U);
	EXPECT_EQ(scaleTicks(123'456'789, 1, 1), 123'456'789U);
}

TEST(TimeTest, ScaleTicksKeepsRemainderOfRatio) {
	EXPECT_EQ(scaleTicks(1, 125, 3), 41U);
	EXPECT_EQ(scaleTicks(2, 125, 3), 83U);
	EXPECT_EQ(scaleTicks(3, 125, 3), 125U);
	EXPECT_EQ(scaleTicks(24'000'000, 125, 3), 1'000'000'000U);
}

TEST(TimeTest, ScaleTicksHandlesUnevenFrequency) {
	constexpr std::uint64_t kFrequency = 3'579'545;

	EXPECT_EQ(scaleTicks(kFrequency, 1'000'000'000, kFrequency), 1'000'000'000U);
	EXPECT_EQ(scaleTicks((kFrequency * 1'000) + 1, 1'000'000'000, kFrequency), 1'000'000'000'279U);
}

TEST(TimeTest, ScaleTicksAvoidsOverflowOnLargeCounts) {
	EXPECT_EQ(scaleTicks(150'000'000'000'000'002, 125, 3), 6'250'000'000'000'000'083U);

	constexpr std::uint64_t kFrequency = 3'579'545;
	EXPECT_EQ(scaleTicks((kFrequency * 2'000'000'000) + kFrequency - 1, 1'000'000'000, kFrequency),
	          2'000'000'000'999'999'720U);
}

TEST(TimeTest, OneSecondOfTicksConvertsToOneSecond) {
	EXPECT_EQ(ticksToDuration(0), Duration{});
	EXPECT_EQ(ticksToDuration(ticksPerSecond()), seconds(1));
	EXPECT_EQ(ticksToDuration(ticksPerSecond() * 60), seconds(60));
}

TEST(TimeTest, NowTicksNeverDecreases) {
	std::uint64_t previous = nowTicks();

	for (std::int32_t i = 0; i < 10'000; ++i) {
		const std::uint64_t kCurrent = nowTicks();
		EXPECT_GE(kCurrent, previous);
		previous = kCurrent;
	}
}

TEST(TimeTest, NowNeverDecreases) {
	Instant previous = now();

	for (std::int32_t i = 0; i < 10'000; ++i) {
		const Instant kCurrent = now();
		EXPECT_GE(kCurrent, previous);
		previous = kCurrent;
	}
}

TEST(TimeTest, NowAdvances) {
	const Instant kStart = now();
	Instant current = kStart;

	while (current == kStart) {
		current = now();
	}

	EXPECT_GT(current - kStart, Duration{});
	EXPECT_LT(current - kStart, milliseconds(100));
}

TEST(TimeTest, NowMatchesConvertedTicks) {
	const Duration kBefore = ticksToDuration(nowTicks());
	const Instant kNow = now();
	const Duration kAfter = ticksToDuration(nowTicks());

	EXPECT_GE(kNow.count(), kBefore.count());
	EXPECT_LE(kNow.count(), kAfter.count());
}

void expectMatchesSteadyClockOver(std::chrono::nanoseconds interval) {
	const Duration kRoundingSlack = nanoseconds(2);

	const Instant kOuterStart = now();
	const std::chrono::steady_clock::time_point kReferenceStart = std::chrono::steady_clock::now();
	const Instant kInnerStart = now();

	std::chrono::steady_clock::time_point referenceEnd = kReferenceStart;
	while (referenceEnd - kReferenceStart < interval) {
		referenceEnd = std::chrono::steady_clock::now();
	}

	const Instant kInnerEnd = now();
	referenceEnd = std::chrono::steady_clock::now();
	const Instant kOuterEnd = now();

	const Duration kReference
	    = nanoseconds(std::chrono::duration_cast<std::chrono::nanoseconds>(referenceEnd - kReferenceStart).count());

	EXPECT_GE(kReference, (kInnerEnd - kInnerStart) - kRoundingSlack);
	EXPECT_LE(kReference, (kOuterEnd - kOuterStart) + kRoundingSlack);
}

TEST(TimeTest, MatchesSteadyClockOverShortInterval) {
	expectMatchesSteadyClockOver(std::chrono::milliseconds(20));
}

TEST(TimeTest, MatchesSteadyClockOverLongInterval) {
	expectMatchesSteadyClockOver(std::chrono::seconds(1));
}

TEST(TimeTest, SleepForWaitsAtLeastDuration) {
	const Instant kStart = now();

	sleepFor(milliseconds(5));

	EXPECT_GE(now() - kStart, milliseconds(5));
}

TEST(TimeTest, SleepUntilReturnsAtOrAfterDeadline) {
	const Instant kDeadline = now() + milliseconds(5);

	sleepUntil(kDeadline);

	EXPECT_GE(now(), kDeadline);
}

TEST(TimeTest, SleepUntilPastDeadlineReturnsImmediately) {
	const Instant kStart = now();

	sleepUntil(kStart - milliseconds(1));
	sleepFor(Duration{});
	sleepFor(milliseconds(-1));

	EXPECT_LT(now() - kStart, milliseconds(50));
}

TEST(TimeTest, SleepForOvershootsByLessThanSystemTick) {
	Duration smallestOvershoot = seconds(1);

	for (std::int32_t i = 0; i < 10; ++i) {
		const Instant kStart = now();
		sleepFor(milliseconds(1));
		const Duration kOvershoot = (now() - kStart) - milliseconds(1);

		if (kOvershoot < smallestOvershoot) {
			smallestOvershoot = kOvershoot;
		}
	}

	EXPECT_LT(smallestOvershoot, milliseconds(2));
}

static_assert(scaleTicks(24'000'000, 125, 3) == 1'000'000'000);
static_assert(scaleTicks(3, 125, 3) == 125);

} // namespace

} // namespace trivial::time

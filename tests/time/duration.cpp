#include <trivial/core/time/duration.h>

#include <cstdint>

#include <gtest/gtest.h>

namespace trivial::time {

namespace {

TEST(DurationTest, DefaultsToZero) {
	const Duration kDuration{};

	EXPECT_EQ(kDuration.count(), 0);
	EXPECT_EQ(kDuration, nanoseconds(0));
}

TEST(DurationTest, FactoriesStoreNanoseconds) {
	EXPECT_EQ(nanoseconds(7).count(), 7);
	EXPECT_EQ(microseconds(7).count(), 7'000);
	EXPECT_EQ(milliseconds(7).count(), 7'000'000);
	EXPECT_EQ(seconds(7).count(), 7'000'000'000);

	EXPECT_EQ(Duration::fromNanoseconds(7), nanoseconds(7));
	EXPECT_EQ(seconds(2), milliseconds(2'000));
	EXPECT_EQ(milliseconds(-3).count(), -3'000'000);
}

TEST(DurationTest, TruncatingAccessorsRoundTowardZero) {
	EXPECT_EQ(nanoseconds(1'999).toMicroseconds(), 1);
	EXPECT_EQ(nanoseconds(1'999'999).toMilliseconds(), 1);
	EXPECT_EQ(nanoseconds(1'999'999'999).toSeconds(), 1);

	EXPECT_EQ(nanoseconds(-1'999).toMicroseconds(), -1);
	EXPECT_EQ(nanoseconds(-1'999'999).toMilliseconds(), -1);
	EXPECT_EQ(nanoseconds(-1'999'999'999).toSeconds(), -1);
}

TEST(DurationTest, MillisecondsCeilRoundsUp) {
	EXPECT_EQ(nanoseconds(0).toMillisecondsCeil(), 0);
	EXPECT_EQ(nanoseconds(1).toMillisecondsCeil(), 1);
	EXPECT_EQ(nanoseconds(1'000'000).toMillisecondsCeil(), 1);
	EXPECT_EQ(nanoseconds(1'000'001).toMillisecondsCeil(), 2);

	EXPECT_EQ(nanoseconds(-1).toMillisecondsCeil(), 0);
	EXPECT_EQ(nanoseconds(-1'000'000).toMillisecondsCeil(), -1);
	EXPECT_EQ(nanoseconds(-1'500'000).toMillisecondsCeil(), -1);
}

TEST(DurationTest, ConvertsToSecondsDouble) {
	EXPECT_DOUBLE_EQ(milliseconds(1'500).toSecondsDouble(), 1.5);
	EXPECT_DOUBLE_EQ(nanoseconds(1).toSecondsDouble(), 1.0e-9);
	EXPECT_DOUBLE_EQ(milliseconds(-250).toSecondsDouble(), -0.25);
}

TEST(DurationTest, SupportsArithmetic) {
	const Duration kLhs = milliseconds(9);
	const Duration kRhs = milliseconds(4);

	EXPECT_EQ(+kLhs, milliseconds(9));
	EXPECT_EQ(-kLhs, milliseconds(-9));
	EXPECT_EQ(kLhs + kRhs, milliseconds(13));
	EXPECT_EQ(kLhs - kRhs, milliseconds(5));
	EXPECT_EQ(kRhs - kLhs, milliseconds(-5));
	EXPECT_EQ(kLhs * 3, milliseconds(27));
	EXPECT_EQ(3 * kLhs, milliseconds(27));
	EXPECT_EQ(kLhs / 3, milliseconds(3));
	EXPECT_EQ(kLhs / kRhs, 2);
	EXPECT_EQ(kLhs % kRhs, milliseconds(1));
}

TEST(DurationTest, SupportsCompoundAssignment) {
	Duration duration = milliseconds(10);
	const Duration kOther = milliseconds(4);

	EXPECT_EQ(&(duration += kOther), &duration);
	EXPECT_EQ(duration, milliseconds(14));

	EXPECT_EQ(&(duration -= kOther), &duration);
	EXPECT_EQ(duration, milliseconds(10));

	EXPECT_EQ(&(duration *= 3), &duration);
	EXPECT_EQ(duration, milliseconds(30));

	EXPECT_EQ(&(duration /= 2), &duration);
	EXPECT_EQ(duration, milliseconds(15));

	EXPECT_EQ(&(duration %= kOther), &duration);
	EXPECT_EQ(duration, milliseconds(3));
}

TEST(DurationTest, AccumulatesFixedSteps) {
	Duration accumulator = milliseconds(25);
	const Duration kStep = milliseconds(8);

	std::int64_t steps = 0;
	while (accumulator >= kStep) {
		accumulator -= kStep;
		++steps;
	}

	EXPECT_EQ(steps, 3);
	EXPECT_EQ(accumulator, milliseconds(1));
	EXPECT_EQ(milliseconds(25) / kStep, steps);
	EXPECT_EQ(milliseconds(25) % kStep, accumulator);
}

TEST(DurationTest, ComparesStoredValue) {
	const Duration kSmaller = milliseconds(1);
	const Duration kLarger = milliseconds(2);
	const Duration kEqual = microseconds(1'000);

	EXPECT_EQ(kSmaller, kEqual);
	EXPECT_NE(kSmaller, kLarger);

	EXPECT_TRUE(kSmaller < kLarger);
	EXPECT_FALSE(kLarger < kSmaller);
	EXPECT_FALSE(kSmaller < kEqual);

	EXPECT_TRUE(kSmaller <= kLarger);
	EXPECT_TRUE(kSmaller <= kEqual);
	EXPECT_FALSE(kLarger <= kSmaller);

	EXPECT_TRUE(kLarger > kSmaller);
	EXPECT_FALSE(kSmaller > kLarger);
	EXPECT_FALSE(kSmaller > kEqual);

	EXPECT_TRUE(kLarger >= kSmaller);
	EXPECT_TRUE(kSmaller >= kEqual);
	EXPECT_FALSE(kSmaller >= kLarger);

	EXPECT_TRUE(milliseconds(-1) < Duration{});
}

static_assert(milliseconds(5).count() == 5'000'000);
static_assert(seconds(1) - milliseconds(1) == microseconds(999'000));

} // namespace

} // namespace trivial::time

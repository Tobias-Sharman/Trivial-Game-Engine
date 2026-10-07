#include <trivial/core/time/instant.h>

#include <cstdint>
#include <limits>

#include <gtest/gtest.h>

#include <trivial/core/time/duration.h>

namespace trivial::time {

namespace {

TEST(InstantTest, DefaultsToZero) {
	const Instant kInstant{};

	EXPECT_EQ(kInstant.count(), 0);
	EXPECT_EQ(kInstant, Instant::fromNanoseconds(0));
}

TEST(InstantTest, FromNanosecondsStoresValue) {
	EXPECT_EQ(Instant::fromNanoseconds(7).count(), 7);
	EXPECT_EQ(Instant::fromNanoseconds(-7).count(), -7);
}

TEST(InstantTest, SubtractingInstantsGivesDuration) {
	const Instant kEarlier = Instant::fromNanoseconds(1'000);
	const Instant kLater = Instant::fromNanoseconds(4'500);

	EXPECT_EQ(kLater - kEarlier, nanoseconds(3'500));
	EXPECT_EQ(kEarlier - kLater, nanoseconds(-3'500));
	EXPECT_EQ(kLater - kLater, Duration{});
}

TEST(InstantTest, OffsettingByDurationGivesInstant) {
	const Instant kStart = Instant::fromNanoseconds(1'000);

	EXPECT_EQ(kStart + microseconds(2), Instant::fromNanoseconds(3'000));
	EXPECT_EQ(microseconds(2) + kStart, Instant::fromNanoseconds(3'000));
	EXPECT_EQ(kStart - microseconds(2), Instant::fromNanoseconds(-1'000));
	EXPECT_EQ((kStart + milliseconds(5)) - kStart, milliseconds(5));
}

TEST(InstantTest, ClampedAddMatchesAddWithinRange) {
	const Instant kStart = Instant::fromNanoseconds(1'000);

	EXPECT_EQ(kStart.clampedAdd(microseconds(2)), kStart + microseconds(2));
	EXPECT_EQ(kStart.clampedAdd(microseconds(-2)), kStart - microseconds(2));
	EXPECT_EQ(kStart.clampedAdd(Duration{}), kStart);
}

TEST(InstantTest, ClampedAddClampsAtLimits) {
	const std::int64_t kMax = std::numeric_limits<std::int64_t>::max();
	const std::int64_t kMin = std::numeric_limits<std::int64_t>::min();

	EXPECT_EQ(Instant::fromNanoseconds(1).clampedAdd(nanoseconds(kMax)), Instant::fromNanoseconds(kMax));
	EXPECT_EQ(Instant::fromNanoseconds(kMax).clampedAdd(nanoseconds(1)), Instant::fromNanoseconds(kMax));
	EXPECT_EQ(Instant::fromNanoseconds(-1).clampedAdd(nanoseconds(kMin)), Instant::fromNanoseconds(kMin));
	EXPECT_EQ(Instant::fromNanoseconds(kMin).clampedAdd(nanoseconds(-1)), Instant::fromNanoseconds(kMin));

	EXPECT_EQ(Instant::fromNanoseconds(0).clampedAdd(nanoseconds(kMax)), Instant::fromNanoseconds(kMax));
	EXPECT_EQ(Instant::fromNanoseconds(0).clampedAdd(nanoseconds(kMin)), Instant::fromNanoseconds(kMin));
}

TEST(InstantTest, SupportsCompoundAssignment) {
	Instant instant = Instant::fromNanoseconds(1'000);

	EXPECT_EQ(&(instant += nanoseconds(500)), &instant);
	EXPECT_EQ(instant, Instant::fromNanoseconds(1'500));

	EXPECT_EQ(&(instant -= nanoseconds(2'000)), &instant);
	EXPECT_EQ(instant, Instant::fromNanoseconds(-500));
}

TEST(InstantTest, ComparesStoredValue) {
	const Instant kEarlier = Instant::fromNanoseconds(1);
	const Instant kLater = Instant::fromNanoseconds(2);
	const Instant kEqual = Instant::fromNanoseconds(1);

	EXPECT_EQ(kEarlier, kEqual);
	EXPECT_NE(kEarlier, kLater);

	EXPECT_TRUE(kEarlier < kLater);
	EXPECT_FALSE(kLater < kEarlier);
	EXPECT_FALSE(kEarlier < kEqual);

	EXPECT_TRUE(kEarlier <= kLater);
	EXPECT_TRUE(kEarlier <= kEqual);
	EXPECT_FALSE(kLater <= kEarlier);

	EXPECT_TRUE(kLater > kEarlier);
	EXPECT_FALSE(kEarlier > kLater);
	EXPECT_FALSE(kEarlier > kEqual);

	EXPECT_TRUE(kLater >= kEarlier);
	EXPECT_TRUE(kEarlier >= kEqual);
	EXPECT_FALSE(kEarlier >= kLater);
}

static_assert(Instant::fromNanoseconds(10) - Instant::fromNanoseconds(4) == nanoseconds(6));
static_assert(Instant{} + seconds(1) == Instant::fromNanoseconds(1'000'000'000));

} // namespace

} // namespace trivial::time

#include <trivial/core/math/angle.h>

#include <type_traits>

#include <gtest/gtest.h>

#include <trivial/core/math/math_constants.h>

namespace trivial::math {

namespace {

template <typename T>
constexpr T epsilon() noexcept {
	if constexpr (std::is_same_v<T, float>) {
		return T{1.0e-5F};
	} else {
		return T{1.0e-12};
	}
}

template <typename T>
class AngleTest : public testing::Test {};

using FloatingPointTypes = testing::Types<float, double>;

TYPED_TEST_SUITE(AngleTest, FloatingPointTypes);

TYPED_TEST(AngleTest, DefaultsToZero) {
	using T = TypeParam;

	const Angle<T> kAngle{};

	EXPECT_EQ(kAngle.radians(), T{});
	EXPECT_EQ(kAngle.degrees(), T{});
}

TYPED_TEST(AngleTest, ConvertsBetweenDegreesAndRadians) {
	using T = TypeParam;

	const Angle<T> kFromDegrees = Angle<T>::fromDegrees(T{180});
	const Angle<T> kFromRadians = Angle<T>::fromRadians(constants::g_kHalfPi<T>);

	EXPECT_NEAR(kFromDegrees.radians(), constants::g_kPi<T>, epsilon<T>());
	EXPECT_NEAR(kFromRadians.degrees(), T{90}, epsilon<T>());

	const Angle<T> kRoundTripDegrees = Angle<T>::fromDegrees(T{123.5});
	const Angle<T> kRoundTripRadians = Angle<T>::fromRadians(T{2.25});

	EXPECT_NEAR(kRoundTripDegrees.degrees(), T{123.5}, epsilon<T>());
	EXPECT_EQ(kRoundTripRadians.radians(), T{2.25});
}

TYPED_TEST(AngleTest, SupportsArithmetic) {
	using T = TypeParam;

	const Angle<T> kLhs = Angle<T>::fromRadians(T{1.5});
	const Angle<T> kRhs = Angle<T>::fromRadians(T{0.5});

	EXPECT_EQ((+kLhs).radians(), T{1.5});
	EXPECT_EQ((-kLhs).radians(), T{-1.5});
	EXPECT_EQ((kLhs + kRhs).radians(), T{2});
	EXPECT_EQ((kLhs - kRhs).radians(), T{1});
	EXPECT_EQ((kLhs * T{2}).radians(), T{3});
	EXPECT_EQ((T{2} * kLhs).radians(), T{3});
	EXPECT_EQ((kLhs / T{2}).radians(), T{0.75});
}

TYPED_TEST(AngleTest, SupportsCompoundAssignment) {
	using T = TypeParam;

	Angle<T> angle = Angle<T>::fromRadians(T{1});
	const Angle<T> kOther = Angle<T>::fromRadians(T{0.5});

	EXPECT_EQ(&(angle += kOther), &angle);
	EXPECT_EQ(angle.radians(), T{1.5});

	EXPECT_EQ(&(angle -= kOther), &angle);
	EXPECT_EQ(angle.radians(), T{1});

	EXPECT_EQ(&(angle *= T{2}), &angle);
	EXPECT_EQ(angle.radians(), T{2});

	EXPECT_EQ(&(angle /= T{2}), &angle);
	EXPECT_EQ(angle.radians(), T{1});
}

TYPED_TEST(AngleTest, ComparesStoredValue) {
	using T = TypeParam;

	const Angle<T> kLhs = Angle<T>::fromRadians(T{1});
	const Angle<T> kEqual = Angle<T>::fromRadians(T{1});
	const Angle<T> kDifferent = Angle<T>::fromRadians(T{2});

	EXPECT_EQ(kLhs, kEqual);
	EXPECT_NE(kLhs, kDifferent);

	const Angle<T> kNearlyEqualAngle = Angle<T>::fromRadians(T{1} + (epsilon<T>() / T{2}));

	EXPECT_TRUE(kLhs.nearlyEqual(kNearlyEqualAngle, epsilon<T>()));
	EXPECT_TRUE(nearlyEqual(kLhs, kNearlyEqualAngle, epsilon<T>()));
	EXPECT_FALSE(kLhs.nearlyEqual(kDifferent, epsilon<T>()));
}

static_assert(sizeof(Anglef) == sizeof(float));
static_assert(sizeof(Angled) == sizeof(double));

static_assert(alignof(Anglef) == alignof(float));
static_assert(alignof(Angled) == alignof(double));

static_assert(std::is_trivially_copyable_v<Anglef>);
static_assert(std::is_trivially_copyable_v<Angled>);

static_assert(std::is_standard_layout_v<Anglef>);
static_assert(std::is_standard_layout_v<Angled>);

static_assert(!std::is_convertible_v<float, Anglef>);
static_assert(!std::is_convertible_v<double, Angled>);

} // namespace

} // namespace trivial::math

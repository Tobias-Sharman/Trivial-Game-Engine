#include <trivial/core/math/angle.h>

#include <type_traits>

#include <gtest/gtest.h>

#include <trivial/core/math/constants.h>

namespace trivial::math {

namespace {

template <typename T>
constexpr T epsilon() noexcept {
	if constexpr (std::is_same_v<T, float>) {
		return T{1.0e-5f};
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

	Angle<T> angle{};

	EXPECT_EQ(angle.radians(), T{});
	EXPECT_EQ(angle.degrees(), T{});
}

TYPED_TEST(AngleTest, ConvertsBetweenDegreesAndRadians) {
	using T = TypeParam;

	Angle<T> fromDegrees = Angle<T>::fromDegrees(T{180});
	Angle<T> fromRadians = Angle<T>::fromRadians(constants::g_kHalfPi<T>);

	EXPECT_NEAR(fromDegrees.radians(), constants::g_kPi<T>, epsilon<T>());
	EXPECT_NEAR(fromRadians.degrees(), T{90}, epsilon<T>());

	Angle<T> roundTripDegrees = Angle<T>::fromDegrees(T{123.5});
	Angle<T> roundTripRadians = Angle<T>::fromRadians(T{2.25});

	EXPECT_NEAR(roundTripDegrees.degrees(), T{123.5}, epsilon<T>());
	EXPECT_EQ(roundTripRadians.radians(), T{2.25});
}

TYPED_TEST(AngleTest, SupportsArithmetic) {
	using T = TypeParam;

	Angle<T> lhs = Angle<T>::fromRadians(T{1.5});
	Angle<T> rhs = Angle<T>::fromRadians(T{0.5});

	EXPECT_EQ((+lhs).radians(), T{1.5});
	EXPECT_EQ((-lhs).radians(), T{-1.5});
	EXPECT_EQ((lhs + rhs).radians(), T{2});
	EXPECT_EQ((lhs - rhs).radians(), T{1});
	EXPECT_EQ((lhs * T{2}).radians(), T{3});
	EXPECT_EQ((T{2} * lhs).radians(), T{3});
	EXPECT_EQ((lhs / T{2}).radians(), T{0.75});
}

TYPED_TEST(AngleTest, SupportsCompoundAssignment) {
	using T = TypeParam;

	Angle<T> angle = Angle<T>::fromRadians(T{1});
	Angle<T> other = Angle<T>::fromRadians(T{0.5});

	EXPECT_EQ(&(angle += other), &angle);
	EXPECT_EQ(angle.radians(), T{1.5});

	EXPECT_EQ(&(angle -= other), &angle);
	EXPECT_EQ(angle.radians(), T{1});

	EXPECT_EQ(&(angle *= T{2}), &angle);
	EXPECT_EQ(angle.radians(), T{2});

	EXPECT_EQ(&(angle /= T{2}), &angle);
	EXPECT_EQ(angle.radians(), T{1});
}

TYPED_TEST(AngleTest, ComparesStoredValue) {
	using T = TypeParam;

	Angle<T> lhs = Angle<T>::fromRadians(T{1});
	Angle<T> equal = Angle<T>::fromRadians(T{1});
	Angle<T> different = Angle<T>::fromRadians(T{2});

	EXPECT_EQ(lhs, equal);
	EXPECT_NE(lhs, different);

	Angle<T> nearlyEqualAngle = Angle<T>::fromRadians(T{1} + (epsilon<T>() / T{2}));

	EXPECT_TRUE(lhs.nearlyEqual(nearlyEqualAngle, epsilon<T>()));
	EXPECT_TRUE(nearlyEqual(lhs, nearlyEqualAngle, epsilon<T>()));
	EXPECT_FALSE(lhs.nearlyEqual(different, epsilon<T>()));
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

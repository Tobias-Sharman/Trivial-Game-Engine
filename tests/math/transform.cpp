#include <type_traits>

#include <gtest/gtest.h>

#include <trivial/core/math/affine2.h>
#include <trivial/core/math/angle.h>
#include <trivial/core/math/transform2.h>
#include <trivial/core/math/vec2.h>

namespace trivial::math {

namespace {

template <typename T>
constexpr T epsilon() noexcept {
	if constexpr (std::is_same_v<T, float>) {
		return T{1.0e-5F};
	} else {
		return T{1.0e-12F};
	}
}

template <typename T>
void expectNear(const Vec2<T>& lhs, const Vec2<T>& rhs) {
	EXPECT_NEAR(lhs.x, rhs.x, epsilon<T>());
	EXPECT_NEAR(lhs.y, rhs.y, epsilon<T>());
}

template <typename T>
void expectNear(const Affine2<T>& lhs, const Affine2<T>& rhs) {
	EXPECT_NEAR(lhs.a, rhs.a, epsilon<T>());
	EXPECT_NEAR(lhs.b, rhs.b, epsilon<T>());
	EXPECT_NEAR(lhs.tx, rhs.tx, epsilon<T>());
	EXPECT_NEAR(lhs.c, rhs.c, epsilon<T>());
	EXPECT_NEAR(lhs.d, rhs.d, epsilon<T>());
	EXPECT_NEAR(lhs.ty, rhs.ty, epsilon<T>());
}

template <typename T>
class Transform2Test : public testing::Test {};

using FloatingPointTypes = testing::Types<float, double>;

TYPED_TEST_SUITE(Transform2Test, FloatingPointTypes);

TYPED_TEST(Transform2Test, DefaultsToIdentity) {
	using T = TypeParam;

	const Transform2<T> kTransform{};

	EXPECT_EQ(kTransform.position, Vec2<T>{});
	EXPECT_EQ(kTransform.rotation, Angle<T>{});
	EXPECT_EQ(kTransform.scale, (Vec2<T>{T{1}, T{1}}));

	EXPECT_EQ(kTransform, Transform2<T>::identity());
	EXPECT_EQ(kTransform.affine(), Affine2<T>::identity());
}

TYPED_TEST(Transform2Test, CreatesAffineTransform) {
	using T = TypeParam;

	const Transform2<T> kTransform{
	    .position = {T{10}, T{5}},
	    .rotation = Angle<T>::fromDegrees(T{90}),
	    .scale = {T{2}, T{3}},
	};
	const Affine2<T> kAffine = kTransform.affine();

	expectNear(kAffine.transformPoint({T{1}, T{2}}), (Vec2<T>{T{4}, T{7}}));
	expectNear(kAffine.transformVector({T{1}, T{2}}), (Vec2<T>{T{-6}, T{2}}));
}

TYPED_TEST(Transform2Test, UsesScaleRotationTranslationOrder) {
	using T = TypeParam;

	const Transform2<T> kTransform{
	    .position = {T{10}, T{5}},
	    .rotation = Angle<T>::fromDegrees(T{90}),
	    .scale = {T{2}, T{3}},
	};
	const Affine2<T> kExpected = Affine2<T>::translation(kTransform.position)
	                             * Affine2<T>::rotation(kTransform.rotation) * Affine2<T>::scale(kTransform.scale);

	expectNear(kTransform.affine(), kExpected);
	expectNear(toAffine(kTransform), kExpected);
}

TYPED_TEST(Transform2Test, ComparesValues) {
	using T = TypeParam;

	const Transform2<T> kLhs{
	    .position = {T{1}, T{2}},
	    .rotation = Angle<T>::fromRadians(T{0.5}),
	    .scale = {T{2}, T{3}},
	};
	const Transform2<T> kEqual = kLhs;
	const Transform2<T> kNearlyEqualValue{
	    .position = {T{1} + (epsilon<T>() / T{2}), T{2}},
	    .rotation = Angle<T>::fromRadians(T{0.5} + (epsilon<T>() / T{2})),
	    .scale = {T{2}, T{3} - (epsilon<T>() / T{2})},
	};
	const Transform2<T> kDifferent{
	    .position = {T{10}, T{20}},
	    .rotation = Angle<T>::fromRadians(T{1}),
	    .scale = {T{4}, T{5}},
	};

	EXPECT_EQ(kLhs, kEqual);
	EXPECT_NE(kLhs, kDifferent);

	EXPECT_TRUE(kLhs.nearlyEqual(kNearlyEqualValue, epsilon<T>()));
	EXPECT_TRUE(nearlyEqual(kLhs, kNearlyEqualValue, epsilon<T>()));
	EXPECT_FALSE(kLhs.nearlyEqual(kDifferent, epsilon<T>()));
}

} // namespace

} // namespace trivial::math

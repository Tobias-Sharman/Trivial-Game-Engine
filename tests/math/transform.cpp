#include <type_traits>

#include <gtest/gtest.h>

#include <trivial/core/math/transform2.h>

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

	Transform2<T> transform{};

	EXPECT_EQ(transform.position, Vec2<T>{});
	EXPECT_EQ(transform.rotation, Angle<T>{});
	EXPECT_EQ(transform.scale, (Vec2<T>{T{1}, T{1}}));

	EXPECT_EQ(transform, Transform2<T>::identity());
	EXPECT_EQ(transform.affine(), Affine2<T>::identity());
}

TYPED_TEST(Transform2Test, CreatesAffineTransform) {
	using T = TypeParam;

	Transform2<T> transform{.position = {T{10}, T{5}}, .rotation = Angle<T>::fromDegrees(T{90}), .scale = {T{2}, T{3}}};
	Affine2<T> affine = transform.affine();

	expectNear(affine.transformPoint({T{1}, T{2}}), (Vec2<T>{T{4}, T{7}}));
	expectNear(affine.transformVector({T{1}, T{2}}), (Vec2<T>{T{-6}, T{2}}));
}

TYPED_TEST(Transform2Test, UsesScaleRotationTranslationOrder) {
	using T = TypeParam;

	Transform2<T> transform{.position = {T{10}, T{5}}, .rotation = Angle<T>::fromDegrees(T{90}), .scale = {T{2}, T{3}}};
	Affine2<T> expected = Affine2<T>::translation(transform.position) * Affine2<T>::rotation(transform.rotation)
	                      * Affine2<T>::scale(transform.scale);

	expectNear(transform.affine(), expected);
	expectNear(toAffine(transform), expected);
}

TYPED_TEST(Transform2Test, ComparesValues) {
	using T = TypeParam;

	Transform2<T> lhs{.position = {T{1}, T{2}}, .rotation = Angle<T>::fromRadians(T{0.5}), .scale = {T{2}, T{3}}};
	Transform2<T> equal = lhs;
	Transform2<T> nearlyEqualValue{.position = {T{1} + (epsilon<T>() / T{2}), T{2}},
	                               .rotation = Angle<T>::fromRadians(T{0.5} + (epsilon<T>() / T{2})),
	                               .scale = {T{2}, T{3} - (epsilon<T>() / T{2})}};
	Transform2<T> different{.position = {T{10}, T{20}}, .rotation = Angle<T>::fromRadians(T{1}), .scale = {T{4}, T{5}}};

	EXPECT_EQ(lhs, equal);
	EXPECT_NE(lhs, different);

	EXPECT_TRUE(lhs.nearlyEqual(nearlyEqualValue, epsilon<T>()));
	EXPECT_TRUE(nearlyEqual(lhs, nearlyEqualValue, epsilon<T>()));
	EXPECT_FALSE(lhs.nearlyEqual(different, epsilon<T>()));
}

static_assert(sizeof(Transform2f) == sizeof(Vec2f) + sizeof(Anglef) + sizeof(Vec2f));
static_assert(sizeof(Transform2d) == sizeof(Vec2d) + sizeof(Angled) + sizeof(Vec2d));

static_assert(alignof(Transform2f) == alignof(float));
static_assert(alignof(Transform2d) == alignof(double));

static_assert(std::is_trivially_copyable_v<Transform2f>);
static_assert(std::is_trivially_copyable_v<Transform2d>);

static_assert(std::is_standard_layout_v<Transform2f>);
static_assert(std::is_standard_layout_v<Transform2d>);

} // namespace
//
} // namespace trivial::math

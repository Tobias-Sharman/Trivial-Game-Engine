#include <type_traits>

#include <gtest/gtest.h>

#include <trivial/core/math/affine2.h>
#include <trivial/core/math/angle.h>
#include <trivial/core/math/mat4.h>
#include <trivial/core/math/vec2.h>
#include <trivial/core/math/vec4.h>

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
void expectNear(const Vec4<T>& lhs, const Vec4<T>& rhs) {
	EXPECT_NEAR(lhs.x, rhs.x, epsilon<T>());
	EXPECT_NEAR(lhs.y, rhs.y, epsilon<T>());
	EXPECT_NEAR(lhs.z, rhs.z, epsilon<T>());
	EXPECT_NEAR(lhs.w, rhs.w, epsilon<T>());
}

template <typename T>
void expectNear(const Mat4<T>& lhs, const Mat4<T>& rhs) {
	expectNear(lhs.col0, rhs.col0);
	expectNear(lhs.col1, rhs.col1);
	expectNear(lhs.col2, rhs.col2);
	expectNear(lhs.col3, rhs.col3);
}

template <typename T>
class Affine2Test : public testing::Test {};

template <typename T>
class Mat4Test : public testing::Test {};

using FloatingPointTypes = testing::Types<float, double>;

TYPED_TEST_SUITE(Affine2Test, FloatingPointTypes);

TYPED_TEST(Affine2Test, CreatesIdentity) {
	using T = TypeParam;

	const Affine2<T> kIdentity = Affine2<T>::identity();
	const Vec2<T> kPoint{T{2}, T{3}};

	EXPECT_EQ(kIdentity, (Affine2<T>{T{1}, T{}, T{}, T{}, T{1}, T{}}));
	EXPECT_EQ(kIdentity.transformPoint(kPoint), kPoint);
	EXPECT_EQ(kIdentity.transformVector(kPoint), kPoint);
	EXPECT_EQ(kIdentity.determinant(), T{1});
}

TYPED_TEST(Affine2Test, CreatesTranslation) {
	using T = TypeParam;

	const Affine2<T> kTranslation = Affine2<T>::translation({T{10}, T{5}});

	EXPECT_EQ(kTranslation.transformPoint({T{2}, T{3}}), (Vec2<T>{T{12}, T{8}}));
	EXPECT_EQ(kTranslation.transformVector({T{2}, T{3}}), (Vec2<T>{T{2}, T{3}}));
	EXPECT_EQ(kTranslation.determinant(), T{1});
}

TYPED_TEST(Affine2Test, CreatesScale) {
	using T = TypeParam;

	const Affine2<T> kNonUniform = Affine2<T>::scale({T{2}, T{3}});
	const Affine2<T> kUniform = Affine2<T>::scale(T{2});

	EXPECT_EQ(kNonUniform.transformPoint({T{4}, T{5}}), (Vec2<T>{T{8}, T{15}}));
	EXPECT_EQ(kUniform.transformPoint({T{4}, T{5}}), (Vec2<T>{T{8}, T{10}}));
	EXPECT_EQ(kNonUniform.determinant(), T{6});
	EXPECT_EQ(kUniform.determinant(), T{4});
}

TYPED_TEST(Affine2Test, CreatesRotation) {
	using T = TypeParam;

	const Affine2<T> kRotation = Affine2<T>::rotation(Angle<T>::fromDegrees(T{90}));

	expectNear(kRotation.transformPoint({T{1}, T{0}}), (Vec2<T>{T{0}, T{1}}));
	expectNear(kRotation.transformVector({T{0}, T{1}}), (Vec2<T>{T{-1}, T{0}}));
	EXPECT_NEAR(kRotation.determinant(), T{1}, epsilon<T>());
}

TYPED_TEST(Affine2Test, ComposesInRightToLeftOrder) {
	using T = TypeParam;

	const Vec2<T> kPoint{T{1}, T{2}};

	const Affine2<T> kScale = Affine2<T>::scale(T{2});
	const Affine2<T> kTranslation = Affine2<T>::translation({T{10}, T{5}});
	const Affine2<T> kCombined = kTranslation * kScale;

	EXPECT_EQ(kCombined.transformPoint(kPoint), (Vec2<T>{T{12}, T{9}}));
	EXPECT_EQ(kCombined.transformPoint(kPoint), kTranslation.transformPoint(kScale.transformPoint(kPoint)));
}

TYPED_TEST(Affine2Test, SupportsCompoundComposition) {
	using T = TypeParam;

	Affine2<T> transform = Affine2<T>::translation({T{10}, T{5}});
	const Affine2<T> kScale = Affine2<T>::scale(T{2});
	const Affine2<T> kExpected = transform * kScale;

	EXPECT_EQ(&(transform *= kScale), &transform);
	EXPECT_EQ(transform, kExpected);
}

TYPED_TEST(Affine2Test, ComparesValues) {
	using T = TypeParam;

	const Affine2<T> kNearlyEqualValue{T{1} + (epsilon<T>() / T{2}), T{}, T{}, T{}, T{1}, T{}};

	const Affine2<T> kLhs = Affine2<T>::identity();
	const Affine2<T> kEqual = Affine2<T>::identity();
	const Affine2<T> kDifferent = Affine2<T>::translation({T{1}, T{2}});

	EXPECT_EQ(kLhs, kEqual);
	EXPECT_NE(kLhs, kDifferent);

	EXPECT_TRUE(kLhs.nearlyEqual(kNearlyEqualValue, epsilon<T>()));
	EXPECT_TRUE(nearlyEqual(kLhs, kNearlyEqualValue, epsilon<T>()));
	EXPECT_FALSE(kLhs.nearlyEqual(kDifferent, epsilon<T>()));
}

static_assert(sizeof(Affine2f) == sizeof(float) * 6);
static_assert(sizeof(Affine2d) == sizeof(double) * 6);

static_assert(alignof(Affine2f) == alignof(float));
static_assert(alignof(Affine2d) == alignof(double));

static_assert(std::is_trivially_copyable_v<Affine2f>);
static_assert(std::is_trivially_copyable_v<Affine2d>);

static_assert(std::is_standard_layout_v<Affine2f>);
static_assert(std::is_standard_layout_v<Affine2d>);

TYPED_TEST_SUITE(Mat4Test, FloatingPointTypes);

TYPED_TEST(Mat4Test, DefaultInitialisesToZero) {
	using T = TypeParam;

	const Mat4<T> kMatrix{};

	EXPECT_EQ(kMatrix.col0, (Vec4<T>{}));
	EXPECT_EQ(kMatrix.col1, (Vec4<T>{}));
	EXPECT_EQ(kMatrix.col2, (Vec4<T>{}));
	EXPECT_EQ(kMatrix.col3, (Vec4<T>{}));
}

TYPED_TEST(Mat4Test, CreatesFromRows) {
	using T = TypeParam;

	const Mat4<T> kMatrix = fromRows(T{1},
	                                 T{2},
	                                 T{3},
	                                 T{4},
	                                 T{5},
	                                 T{6},
	                                 T{7},
	                                 T{8},
	                                 T{9},
	                                 T{10},
	                                 T{11},
	                                 T{12},
	                                 T{13},
	                                 T{14},
	                                 T{15},
	                                 T{16});

	EXPECT_EQ(kMatrix.col0, (Vec4<T>{T{1}, T{5}, T{9}, T{13}}));
	EXPECT_EQ(kMatrix.col1, (Vec4<T>{T{2}, T{6}, T{10}, T{14}}));
	EXPECT_EQ(kMatrix.col2, (Vec4<T>{T{3}, T{7}, T{11}, T{15}}));
	EXPECT_EQ(kMatrix.col3, (Vec4<T>{T{4}, T{8}, T{12}, T{16}}));
}

TYPED_TEST(Mat4Test, IndexesColumns) {
	using T = TypeParam;

	Mat4<T> matrix = fromRows(T{1},
	                          T{2},
	                          T{3},
	                          T{4},
	                          T{5},
	                          T{6},
	                          T{7},
	                          T{8},
	                          T{9},
	                          T{10},
	                          T{11},
	                          T{12},
	                          T{13},
	                          T{14},
	                          T{15},
	                          T{16});

	EXPECT_EQ(matrix[0], matrix.col0);
	EXPECT_EQ(matrix[1], matrix.col1);
	EXPECT_EQ(matrix[2], matrix.col2);
	EXPECT_EQ(matrix[3], matrix.col3);

	matrix[2] = Vec4<T>{T{20}, T{21}, T{22}, T{23}};

	EXPECT_EQ(matrix.col2, (Vec4<T>{T{20}, T{21}, T{22}, T{23}}));
}

TYPED_TEST(Mat4Test, CreatesIdentity) {
	using T = TypeParam;

	const Mat4<T> kIdentity = Mat4<T>::identity();
	const Vec4<T> kVector{T{2}, T{3}, T{4}, T{1}};

	const Mat4<T> kExpected
	    = fromRows(T{1}, T{}, T{}, T{}, T{}, T{1}, T{}, T{}, T{}, T{}, T{1}, T{}, T{}, T{}, T{}, T{1});

	EXPECT_EQ(kIdentity, kExpected);
	EXPECT_EQ(kIdentity * kVector, kVector);
	EXPECT_EQ(kIdentity * kIdentity, kIdentity);
}

TYPED_TEST(Mat4Test, CreatesTranslation) {
	using T = TypeParam;

	const Mat4<T> kTranslation = Mat4<T>::translation({T{10}, T{5}, T{2}});
	const Mat4<T> kExpected
	    = fromRows(T{1}, T{}, T{}, T{10}, T{}, T{1}, T{}, T{5}, T{}, T{}, T{1}, T{2}, T{}, T{}, T{}, T{1});

	const Vec4<T> kPoint{T{2}, T{3}, T{4}, T{1}};
	const Vec4<T> kExpectedPoint{T{12}, T{8}, T{6}, T{1}};

	const Vec4<T> kVector{T{2}, T{3}, T{4}, T{0}};
	const Vec4<T> kExpectedVector{T{2}, T{3}, T{4}, T{0}};

	EXPECT_EQ(kTranslation, kExpected);
	EXPECT_EQ(kTranslation * kPoint, kExpectedPoint);
	EXPECT_EQ(kTranslation * kVector, kExpectedVector);
}

TYPED_TEST(Mat4Test, CreatesScale) {
	using T = TypeParam;

	const Mat4<T> kScale = Mat4<T>::scale({T{2}, T{3}, T{4}});

	const Mat4<T> kExpected
	    = fromRows(T{2}, T{}, T{}, T{}, T{}, T{3}, T{}, T{}, T{}, T{}, T{4}, T{}, T{}, T{}, T{}, T{1});

	const Vec4<T> kVector{T{4}, T{5}, T{6}, T{1}};
	const Vec4<T> kExpectedVector{T{8}, T{15}, T{24}, T{1}};

	EXPECT_EQ(kScale, kExpected);
	EXPECT_EQ(kScale * kVector, kExpectedVector);
}

TYPED_TEST(Mat4Test, CreatesRotationX) {
	using T = TypeParam;

	const Mat4<T> kRotation = Mat4<T>::rotationX(Angle<T>::fromDegrees(T{90}));

	expectNear(kRotation * Vec4<T>{T{0}, T{1}, T{0}, T{0}}, (Vec4<T>{T{0}, T{0}, T{1}, T{0}}));
	expectNear(kRotation * Vec4<T>{T{0}, T{0}, T{1}, T{0}}, (Vec4<T>{T{0}, T{-1}, T{0}, T{0}}));
}

TYPED_TEST(Mat4Test, CreatesRotationY) {
	using T = TypeParam;

	const Mat4<T> kRotation = Mat4<T>::rotationY(Angle<T>::fromDegrees(T{90}));

	expectNear(kRotation * Vec4<T>{T{1}, T{0}, T{0}, T{0}}, (Vec4<T>{T{0}, T{0}, T{-1}, T{0}}));
	expectNear(kRotation * Vec4<T>{T{0}, T{0}, T{1}, T{0}}, (Vec4<T>{T{1}, T{0}, T{0}, T{0}}));
}

TYPED_TEST(Mat4Test, CreatesRotationZ) {
	using T = TypeParam;

	const Mat4<T> kRotation = Mat4<T>::rotationZ(Angle<T>::fromDegrees(T{90}));
	expectNear(kRotation * Vec4<T>{T{1}, T{0}, T{0}, T{0}}, (Vec4<T>{T{0}, T{1}, T{0}, T{0}}));
	expectNear(kRotation * Vec4<T>{T{0}, T{1}, T{0}, T{0}}, (Vec4<T>{T{-1}, T{0}, T{0}, T{0}}));
}

TYPED_TEST(Mat4Test, MultipliesVector) {
	using T = TypeParam;

	const Mat4<T> kMatrix = fromRows(T{1},
	                                 T{2},
	                                 T{3},
	                                 T{4},
	                                 T{5},
	                                 T{6},
	                                 T{7},
	                                 T{8},
	                                 T{9},
	                                 T{10},
	                                 T{11},
	                                 T{12},
	                                 T{13},
	                                 T{14},
	                                 T{15},
	                                 T{16});

	const Vec4<T> kVector{T{1}, T{2}, T{3}, T{4}};

	EXPECT_EQ(kMatrix * kVector, (Vec4<T>{T{30}, T{70}, T{110}, T{150}}));
}

TYPED_TEST(Mat4Test, MultipliesMatrices) {
	using T = TypeParam;

	const Mat4<T> kLhs = fromRows(T{1},
	                              T{2},
	                              T{3},
	                              T{4},
	                              T{5},
	                              T{6},
	                              T{7},
	                              T{8},
	                              T{9},
	                              T{10},
	                              T{11},
	                              T{12},
	                              T{13},
	                              T{14},
	                              T{15},
	                              T{16});

	const Mat4<T> kRhs = fromRows(T{17},
	                              T{18},
	                              T{19},
	                              T{20},
	                              T{21},
	                              T{22},
	                              T{23},
	                              T{24},
	                              T{25},
	                              T{26},
	                              T{27},
	                              T{28},
	                              T{29},
	                              T{30},
	                              T{31},
	                              T{32});

	const Mat4<T> kExpected = fromRows(T{250},
	                                   T{260},
	                                   T{270},
	                                   T{280},
	                                   T{618},
	                                   T{644},
	                                   T{670},
	                                   T{696},
	                                   T{986},
	                                   T{1028},
	                                   T{1070},
	                                   T{1112},
	                                   T{1354},
	                                   T{1412},
	                                   T{1470},
	                                   T{1528});

	EXPECT_EQ(kLhs * kRhs, kExpected);
}

TYPED_TEST(Mat4Test, ComposesInRightToLeftOrder) {
	using T = TypeParam;

	const Vec4<T> kPoint{T{1}, T{2}, T{3}, T{1}};

	const Mat4<T> kScale = Mat4<T>::scale({T{2}, T{3}, T{4}});
	const Mat4<T> kTranslation = Mat4<T>::translation({T{10}, T{5}, T{2}});
	const Mat4<T> kCombined = kTranslation * kScale;

	EXPECT_EQ(kCombined * kPoint, (Vec4<T>{T{12}, T{11}, T{14}, T{1}}));
	EXPECT_EQ(kCombined * kPoint, kTranslation * (kScale * kPoint));
}

TYPED_TEST(Mat4Test, TransposesMatrix) {
	using T = TypeParam;

	const Mat4<T> kMatrix = fromRows(T{1},
	                                 T{2},
	                                 T{3},
	                                 T{4},
	                                 T{5},
	                                 T{6},
	                                 T{7},
	                                 T{8},
	                                 T{9},
	                                 T{10},
	                                 T{11},
	                                 T{12},
	                                 T{13},
	                                 T{14},
	                                 T{15},
	                                 T{16});

	const Mat4<T> kExpected = fromRows(T{1},
	                                   T{5},
	                                   T{9},
	                                   T{13},
	                                   T{2},
	                                   T{6},
	                                   T{10},
	                                   T{14},
	                                   T{3},
	                                   T{7},
	                                   T{11},
	                                   T{15},
	                                   T{4},
	                                   T{8},
	                                   T{12},
	                                   T{16});

	EXPECT_EQ(transpose(kMatrix), kExpected);
	EXPECT_EQ(transpose(transpose(kMatrix)), kMatrix);
}

TYPED_TEST(Mat4Test, ComparesValues) {
	using T = TypeParam;

	Mat4<T> nearlyEqualValue = Mat4<T>::identity();
	nearlyEqualValue.col0.x += epsilon<T>() / T{2};

	const Mat4<T> kLhs = Mat4<T>::identity();
	const Mat4<T> kEqual = Mat4<T>::identity();
	const Mat4<T> kDifferent = Mat4<T>::translation({T{1}, T{2}, T{3}});

	EXPECT_EQ(kLhs, kEqual);
	EXPECT_NE(kLhs, kDifferent);

	EXPECT_TRUE(kLhs.nearlyEqual(nearlyEqualValue, epsilon<T>()));
	EXPECT_TRUE(nearlyEqual(kLhs, nearlyEqualValue, epsilon<T>()));
	EXPECT_FALSE(kLhs.nearlyEqual(kDifferent, epsilon<T>()));
}

constexpr Mat4f g_kConstexprMatrix
    = fromRows(1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F, 7.0F, 8.0F, 9.0F, 10.0F, 11.0F, 12.0F, 13.0F, 14.0F, 15.0F, 16.0F);

static_assert(g_kConstexprMatrix * Vec4f{.x = 1.0F, .y = 2.0F, .z = 3.0F, .w = 4.0F}
              == Vec4f{.x = 30.0F, .y = 70.0F, .z = 110.0F, .w = 150.0F});
static_assert(Mat4f::identity() * g_kConstexprMatrix == g_kConstexprMatrix);
static_assert(g_kConstexprMatrix * Mat4f::identity() == g_kConstexprMatrix);

static_assert(sizeof(Mat4f) == sizeof(float) * 16);
static_assert(sizeof(Mat4d) == sizeof(double) * 16);

static_assert(alignof(Mat4f) == alignof(float));
static_assert(alignof(Mat4d) == alignof(double));

static_assert(std::is_aggregate_v<Mat4f>);
static_assert(std::is_aggregate_v<Mat4d>);

static_assert(std::is_trivially_copyable_v<Mat4f>);
static_assert(std::is_trivially_copyable_v<Mat4d>);

static_assert(std::is_standard_layout_v<Mat4f>);
static_assert(std::is_standard_layout_v<Mat4d>);

} // namespace

} // namespace trivial::math

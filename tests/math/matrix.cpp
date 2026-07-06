#include <type_traits>

#include <gtest/gtest.h>

#include <trivial/core/math/affine2.h>
#include <trivial/core/math/mat4.h>

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

	Affine2<T> identity = Affine2<T>::identity();
	Vec2<T> point{T{2}, T{3}};

	EXPECT_EQ(identity, (Affine2<T>{T{1}, T{}, T{}, T{}, T{1}, T{}}));
	EXPECT_EQ(identity.transformPoint(point), point);
	EXPECT_EQ(identity.transformVector(point), point);
	EXPECT_EQ(identity.determinant(), T{1});
}

TYPED_TEST(Affine2Test, CreatesTranslation) {
	using T = TypeParam;

	Affine2<T> translation = Affine2<T>::translation({T{10}, T{5}});

	EXPECT_EQ(translation.transformPoint({T{2}, T{3}}), (Vec2<T>{T{12}, T{8}}));
	EXPECT_EQ(translation.transformVector({T{2}, T{3}}), (Vec2<T>{T{2}, T{3}}));
	EXPECT_EQ(translation.determinant(), T{1});
}

TYPED_TEST(Affine2Test, CreatesScale) {
	using T = TypeParam;

	Affine2<T> nonUniform = Affine2<T>::scale({T{2}, T{3}});
	Affine2<T> uniform = Affine2<T>::scale(T{2});

	EXPECT_EQ(nonUniform.transformPoint({T{4}, T{5}}), (Vec2<T>{T{8}, T{15}}));
	EXPECT_EQ(uniform.transformPoint({T{4}, T{5}}), (Vec2<T>{T{8}, T{10}}));
	EXPECT_EQ(nonUniform.determinant(), T{6});
	EXPECT_EQ(uniform.determinant(), T{4});
}

TYPED_TEST(Affine2Test, CreatesRotation) {
	using T = TypeParam;

	Affine2<T> rotation = Affine2<T>::rotation(Angle<T>::fromDegrees(T{90}));

	expectNear(rotation.transformPoint({T{1}, T{0}}), (Vec2<T>{T{0}, T{1}}));
	expectNear(rotation.transformVector({T{0}, T{1}}), (Vec2<T>{T{-1}, T{0}}));
	EXPECT_NEAR(rotation.determinant(), T{1}, epsilon<T>());
}

TYPED_TEST(Affine2Test, ComposesInRightToLeftOrder) {
	using T = TypeParam;

	Vec2<T> point{T{1}, T{2}};

	Affine2<T> scale = Affine2<T>::scale(T{2});
	Affine2<T> translation = Affine2<T>::translation({T{10}, T{5}});
	Affine2<T> combined = translation * scale;

	EXPECT_EQ(combined.transformPoint(point), (Vec2<T>{T{12}, T{9}}));
	EXPECT_EQ(combined.transformPoint(point), translation.transformPoint(scale.transformPoint(point)));
}

TYPED_TEST(Affine2Test, SupportsCompoundComposition) {
	using T = TypeParam;

	Affine2<T> transform = Affine2<T>::translation({T{10}, T{5}});
	Affine2<T> scale = Affine2<T>::scale(T{2});
	Affine2<T> expected = transform * scale;

	EXPECT_EQ(&(transform *= scale), &transform);
	EXPECT_EQ(transform, expected);
}

TYPED_TEST(Affine2Test, ComparesValues) {
	using T = TypeParam;

	Affine2<T> nearlyEqualValue{T{1} + (epsilon<T>() / T{2}), T{}, T{}, T{}, T{1}, T{}};

	Affine2<T> lhs = Affine2<T>::identity();
	Affine2<T> equal = Affine2<T>::identity();
	Affine2<T> different = Affine2<T>::translation({T{1}, T{2}});

	EXPECT_EQ(lhs, equal);
	EXPECT_NE(lhs, different);

	EXPECT_TRUE(lhs.nearlyEqual(nearlyEqualValue, epsilon<T>()));
	EXPECT_TRUE(nearlyEqual(lhs, nearlyEqualValue, epsilon<T>()));
	EXPECT_FALSE(lhs.nearlyEqual(different, epsilon<T>()));
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

	Mat4<T> matrix{};

	EXPECT_EQ(matrix.col0, (Vec4<T>{}));
	EXPECT_EQ(matrix.col1, (Vec4<T>{}));
	EXPECT_EQ(matrix.col2, (Vec4<T>{}));
	EXPECT_EQ(matrix.col3, (Vec4<T>{}));
}

TYPED_TEST(Mat4Test, CreatesFromRows) {
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

	EXPECT_EQ(matrix.col0, (Vec4<T>{T{1}, T{5}, T{9}, T{13}}));
	EXPECT_EQ(matrix.col1, (Vec4<T>{T{2}, T{6}, T{10}, T{14}}));
	EXPECT_EQ(matrix.col2, (Vec4<T>{T{3}, T{7}, T{11}, T{15}}));
	EXPECT_EQ(matrix.col3, (Vec4<T>{T{4}, T{8}, T{12}, T{16}}));
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

	Mat4<T> identity = Mat4<T>::identity();
	Vec4<T> vector{T{2}, T{3}, T{4}, T{1}};

	Mat4<T> expected = fromRows(T{1}, T{}, T{}, T{}, T{}, T{1}, T{}, T{}, T{}, T{}, T{1}, T{}, T{}, T{}, T{}, T{1});

	EXPECT_EQ(identity, expected);
	EXPECT_EQ(identity * vector, vector);
	EXPECT_EQ(identity * identity, identity);
}

TYPED_TEST(Mat4Test, CreatesTranslation) {
	using T = TypeParam;

	Mat4<T> translation = Mat4<T>::translation({T{10}, T{5}, T{2}});
	Mat4<T> expected = fromRows(T{1}, T{}, T{}, T{10}, T{}, T{1}, T{}, T{5}, T{}, T{}, T{1}, T{2}, T{}, T{}, T{}, T{1});

	Vec4<T> point{T{2}, T{3}, T{4}, T{1}};
	Vec4<T> expectedPoint{T{12}, T{8}, T{6}, T{1}};

	Vec4<T> vector{T{2}, T{3}, T{4}, T{0}};
	Vec4<T> expectedVector{T{2}, T{3}, T{4}, T{0}};

	EXPECT_EQ(translation, expected);
	EXPECT_EQ(translation * point, expectedPoint);
	EXPECT_EQ(translation * vector, expectedVector);
}

TYPED_TEST(Mat4Test, CreatesScale) {
	using T = TypeParam;

	Mat4<T> scale = Mat4<T>::scale({T{2}, T{3}, T{4}});

	Mat4<T> expected = fromRows(T{2}, T{}, T{}, T{}, T{}, T{3}, T{}, T{}, T{}, T{}, T{4}, T{}, T{}, T{}, T{}, T{1});

	Vec4<T> vector{T{4}, T{5}, T{6}, T{1}};
	Vec4<T> expectedVector{T{8}, T{15}, T{24}, T{1}};

	EXPECT_EQ(scale, expected);
	EXPECT_EQ(scale * vector, expectedVector);
}

TYPED_TEST(Mat4Test, CreatesRotationX) {
	using T = TypeParam;

	Mat4<T> rotation = Mat4<T>::rotationX(Angle<T>::fromDegrees(T{90}));

	expectNear(rotation * Vec4<T>{T{0}, T{1}, T{0}, T{0}}, (Vec4<T>{T{0}, T{0}, T{1}, T{0}}));
	expectNear(rotation * Vec4<T>{T{0}, T{0}, T{1}, T{0}}, (Vec4<T>{T{0}, T{-1}, T{0}, T{0}}));
}

TYPED_TEST(Mat4Test, CreatesRotationY) {
	using T = TypeParam;

	Mat4<T> rotation = Mat4<T>::rotationY(Angle<T>::fromDegrees(T{90}));

	expectNear(rotation * Vec4<T>{T{1}, T{0}, T{0}, T{0}}, (Vec4<T>{T{0}, T{0}, T{-1}, T{0}}));
	expectNear(rotation * Vec4<T>{T{0}, T{0}, T{1}, T{0}}, (Vec4<T>{T{1}, T{0}, T{0}, T{0}}));
}

TYPED_TEST(Mat4Test, CreatesRotationZ) {
	using T = TypeParam;

	Mat4<T> rotation = Mat4<T>::rotationZ(Angle<T>::fromDegrees(T{90}));
	expectNear(rotation * Vec4<T>{T{1}, T{0}, T{0}, T{0}}, (Vec4<T>{T{0}, T{1}, T{0}, T{0}}));
	expectNear(rotation * Vec4<T>{T{0}, T{1}, T{0}, T{0}}, (Vec4<T>{T{-1}, T{0}, T{0}, T{0}}));
}

TYPED_TEST(Mat4Test, MultipliesVector) {
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

	Vec4<T> vector{T{1}, T{2}, T{3}, T{4}};

	EXPECT_EQ(matrix * vector, (Vec4<T>{T{30}, T{70}, T{110}, T{150}}));
}

TYPED_TEST(Mat4Test, MultipliesMatrices) {
	using T = TypeParam;

	Mat4<T> lhs = fromRows(T{1},
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

	Mat4<T> rhs = fromRows(T{17},
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

	Mat4<T> expected = fromRows(T{250},
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

	EXPECT_EQ(lhs * rhs, expected);
}

TYPED_TEST(Mat4Test, ComposesInRightToLeftOrder) {
	using T = TypeParam;

	Vec4<T> point{T{1}, T{2}, T{3}, T{1}};

	Mat4<T> scale = Mat4<T>::scale({T{2}, T{3}, T{4}});
	Mat4<T> translation = Mat4<T>::translation({T{10}, T{5}, T{2}});
	Mat4<T> combined = translation * scale;

	EXPECT_EQ(combined * point, (Vec4<T>{T{12}, T{11}, T{14}, T{1}}));
	EXPECT_EQ(combined * point, translation * (scale * point));
}

TYPED_TEST(Mat4Test, TransposesMatrix) {
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

	Mat4<T> expected = fromRows(T{1},
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

	EXPECT_EQ(transpose(matrix), expected);
	EXPECT_EQ(transpose(transpose(matrix)), matrix);
}

TYPED_TEST(Mat4Test, ComparesValues) {
	using T = TypeParam;

	Mat4<T> nearlyEqualValue = Mat4<T>::identity();
	nearlyEqualValue.col0.x += epsilon<T>() / T{2};

	Mat4<T> lhs = Mat4<T>::identity();
	Mat4<T> equal = Mat4<T>::identity();
	Mat4<T> different = Mat4<T>::translation({T{1}, T{2}, T{3}});

	EXPECT_EQ(lhs, equal);
	EXPECT_NE(lhs, different);

	EXPECT_TRUE(lhs.nearlyEqual(nearlyEqualValue, epsilon<T>()));
	EXPECT_TRUE(nearlyEqual(lhs, nearlyEqualValue, epsilon<T>()));
	EXPECT_FALSE(lhs.nearlyEqual(different, epsilon<T>()));
}

constexpr Mat4f g_kConstexprMatrix
    = fromRows(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f);

static_assert(g_kConstexprMatrix * Vec4f{1.0f, 2.0f, 3.0f, 4.0f} == Vec4f{30.0f, 70.0f, 110.0f, 150.0f});
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

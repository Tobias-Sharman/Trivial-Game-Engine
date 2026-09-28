#include <type_traits>

#include <gtest/gtest.h>

#include <trivial/core/math/vec2.h>
#include <trivial/core/math/vec3.h>
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
void expectNear(const Vec3<T>& lhs, const Vec3<T>& rhs) {
	EXPECT_NEAR(lhs.x, rhs.x, epsilon<T>());
	EXPECT_NEAR(lhs.y, rhs.y, epsilon<T>());
	EXPECT_NEAR(lhs.z, rhs.z, epsilon<T>());
}

template <typename T>
void expectNear(const Vec4<T>& lhs, const Vec4<T>& rhs) {
	EXPECT_NEAR(lhs.x, rhs.x, epsilon<T>());
	EXPECT_NEAR(lhs.y, rhs.y, epsilon<T>());
	EXPECT_NEAR(lhs.z, rhs.z, epsilon<T>());
	EXPECT_NEAR(lhs.w, rhs.w, epsilon<T>());
}

template <typename T>
void testVec2Basics() {
	Vec2<T> vector{T{1}, T{2}};

	EXPECT_EQ(vector[0], T{1});
	EXPECT_EQ(vector[1], T{2});

	vector[0] = T{3};
	vector[1] = T{4};

	EXPECT_EQ(vector, (Vec2<T>{T{3}, T{4}}));
}

template <typename T>
void testVec2Arithmetic() {
	const Vec2<T> kLhs{T{4}, T{8}};
	const Vec2<T> kRhs{T{2}, T{4}};

	EXPECT_EQ(+kLhs, kLhs);
	EXPECT_EQ(-kLhs, (Vec2<T>{T{-4}, T{-8}}));

	EXPECT_EQ(kLhs + kRhs, (Vec2<T>{T{6}, T{12}}));
	EXPECT_EQ(kLhs - kRhs, (Vec2<T>{T{2}, T{4}}));
	EXPECT_EQ(kLhs * kRhs, (Vec2<T>{T{8}, T{32}}));
	EXPECT_EQ(kLhs / kRhs, (Vec2<T>{T{2}, T{2}}));

	EXPECT_EQ(kLhs * T{2}, (Vec2<T>{T{8}, T{16}}));
	EXPECT_EQ(T{2} * kLhs, (Vec2<T>{T{8}, T{16}}));
	EXPECT_EQ(kLhs / T{2}, (Vec2<T>{T{2}, T{4}}));

	Vec2<T> value = kLhs;

	EXPECT_EQ(&(value += kRhs), &value);
	EXPECT_EQ(value, (Vec2<T>{T{6}, T{12}}));

	EXPECT_EQ(&(value -= kRhs), &value);
	EXPECT_EQ(value, kLhs);

	EXPECT_EQ(&(value *= kRhs), &value);
	EXPECT_EQ(value, (Vec2<T>{T{8}, T{32}}));

	EXPECT_EQ(&(value /= kRhs), &value);
	EXPECT_EQ(value, kLhs);

	EXPECT_EQ(&(value *= T{2}), &value);
	EXPECT_EQ(value, (Vec2<T>{T{8}, T{16}}));

	EXPECT_EQ(&(value /= T{2}), &value);
	EXPECT_EQ(value, kLhs);
}

template <typename T>
void testVec2Geometry() {
	const Vec2<T> kVector{T{3}, T{4}};
	const Vec2<T> kOther{T{2}, T{1}};

	EXPECT_EQ(kVector.dot(kOther), T{10});
	EXPECT_EQ(dot(kVector, kOther), T{10});

	EXPECT_EQ(kVector.cross(kOther), T{-5});
	EXPECT_EQ(cross(kVector, kOther), T{-5});

	EXPECT_EQ(kVector.lengthSquared(), T{25});
	EXPECT_NEAR(kVector.length(), T{5}, epsilon<T>());
	EXPECT_NEAR(kVector.robustLength(), T{5}, epsilon<T>());

	expectNear(kVector.normalised(), (Vec2<T>{T{0.6}, T{0.8}}));
	EXPECT_EQ(Vec2<T>{}.normalisedOrZero(), Vec2<T>{});

	EXPECT_EQ(kVector.perpLeft(), (Vec2<T>{T{-4}, T{3}}));
	EXPECT_EQ(kVector.perpRight(), (Vec2<T>{T{4}, T{-3}}));

	const Vec2<T> kStart{T{0}, T{2}};
	const Vec2<T> kEnd{T{4}, T{6}};

	EXPECT_EQ(kStart.lerp(kEnd, T{0.5}), (Vec2<T>{T{2}, T{4}}));
	EXPECT_EQ(lerp(kStart, kEnd, T{0.5}), (Vec2<T>{T{2}, T{4}}));

	const Vec2<T> kClose{T{1}, T{2}};
	const Vec2<T> kNearlyClose{T{1} + (epsilon<T>() / T{2}), T{2} - (epsilon<T>() / T{2})};

	EXPECT_TRUE(kClose.nearlyEqual(kNearlyClose, epsilon<T>()));
	EXPECT_TRUE(nearlyEqual(kClose, kNearlyClose, epsilon<T>()));
	EXPECT_FALSE(kClose.nearlyEqual(Vec2<T>{T{2}, T{3}}, epsilon<T>()));
}

TEST(Vec2Test, SupportsBasicAccess) {
	testVec2Basics<int>();
	testVec2Basics<float>();
	testVec2Basics<double>();
}

TEST(Vec2Test, SupportsArithmetic) {
	testVec2Arithmetic<int>();
	testVec2Arithmetic<float>();
	testVec2Arithmetic<double>();
}

TEST(Vec2Test, SupportsGeometry) {
	testVec2Geometry<float>();
	testVec2Geometry<double>();
}

template <typename T>
void testVec3Basics() {
	Vec3<T> vector{T{1}, T{2}, T{3}};

	EXPECT_EQ(vector[0], T{1});
	EXPECT_EQ(vector[1], T{2});
	EXPECT_EQ(vector[2], T{3});

	vector[0] = T{4};
	vector[1] = T{5};
	vector[2] = T{6};

	EXPECT_EQ(vector, (Vec3<T>{T{4}, T{5}, T{6}}));
}

template <typename T>
void testVec3Arithmetic() {
	const Vec3<T> kLhs{T{4}, T{8}, T{12}};
	const Vec3<T> kRhs{T{2}, T{4}, T{6}};

	EXPECT_EQ(+kLhs, kLhs);
	EXPECT_EQ(-kLhs, (Vec3<T>{T{-4}, T{-8}, T{-12}}));

	EXPECT_EQ(kLhs + kRhs, (Vec3<T>{T{6}, T{12}, T{18}}));
	EXPECT_EQ(kLhs - kRhs, (Vec3<T>{T{2}, T{4}, T{6}}));
	EXPECT_EQ(kLhs * kRhs, (Vec3<T>{T{8}, T{32}, T{72}}));
	EXPECT_EQ(kLhs / kRhs, (Vec3<T>{T{2}, T{2}, T{2}}));

	EXPECT_EQ(kLhs * T{2}, (Vec3<T>{T{8}, T{16}, T{24}}));
	EXPECT_EQ(T{2} * kLhs, (Vec3<T>{T{8}, T{16}, T{24}}));
	EXPECT_EQ(kLhs / T{2}, (Vec3<T>{T{2}, T{4}, T{6}}));

	Vec3<T> value = kLhs;

	EXPECT_EQ(&(value += kRhs), &value);
	EXPECT_EQ(&(value -= kRhs), &value);
	EXPECT_EQ(&(value *= kRhs), &value);
	EXPECT_EQ(&(value /= kRhs), &value);
	EXPECT_EQ(&(value *= T{2}), &value);
	EXPECT_EQ(&(value /= T{2}), &value);

	EXPECT_EQ(value, kLhs);
}

template <typename T>
void testVec3Geometry() {
	const Vec3<T> kVector{T{3}, T{4}, T{0}};
	const Vec3<T> kOther{T{2}, T{1}, T{3}};

	EXPECT_EQ(kVector.dot(kOther), T{10});
	EXPECT_EQ(dot(kVector, kOther), T{10});

	EXPECT_EQ(kVector.cross(kOther), (Vec3<T>{T{12}, T{-9}, T{-5}}));
	EXPECT_EQ(cross(kVector, kOther), (Vec3<T>{T{12}, T{-9}, T{-5}}));

	EXPECT_EQ(kVector.lengthSquared(), T{25});
	EXPECT_NEAR(kVector.length(), T{5}, epsilon<T>());
	EXPECT_NEAR(kVector.robustLength(), T{5}, epsilon<T>());

	expectNear(kVector.normalised(), (Vec3<T>{T{0.6}, T{0.8}, T{0}}));
	EXPECT_EQ(Vec3<T>{}.normalisedOrZero(), Vec3<T>{});

	const Vec3<T> kStart{T{0}, T{2}, T{4}};
	const Vec3<T> kEnd{T{4}, T{6}, T{8}};

	EXPECT_EQ(kStart.lerp(kEnd, T{0.5}), (Vec3<T>{T{2}, T{4}, T{6}}));
	EXPECT_EQ(lerp(kStart, kEnd, T{0.5}), (Vec3<T>{T{2}, T{4}, T{6}}));

	const Vec3<T> kClose{T{1}, T{2}, T{3}};
	const Vec3<T> kNearlyClose{T{1} + (epsilon<T>() / T{2}), T{2} - (epsilon<T>() / T{2}), T{3}};

	EXPECT_TRUE(kClose.nearlyEqual(kNearlyClose, epsilon<T>()));
	EXPECT_TRUE(nearlyEqual(kClose, kNearlyClose, epsilon<T>()));
	EXPECT_FALSE(kClose.nearlyEqual(Vec3<T>{T{2}, T{3}, T{4}}, epsilon<T>()));
}

TEST(Vec3Test, SupportsBasicAccess) {
	testVec3Basics<int>();
	testVec3Basics<float>();
	testVec3Basics<double>();
}

TEST(Vec3Test, SupportsArithmetic) {
	testVec3Arithmetic<int>();
	testVec3Arithmetic<float>();
	testVec3Arithmetic<double>();
}

TEST(Vec3Test, SupportsGeometry) {
	testVec3Geometry<float>();
	testVec3Geometry<double>();
}

template <typename T>
void testVec4Basics() {
	Vec4<T> vector{T{1}, T{2}, T{3}, T{4}};

	EXPECT_EQ(vector[0], T{1});
	EXPECT_EQ(vector[1], T{2});
	EXPECT_EQ(vector[2], T{3});
	EXPECT_EQ(vector[3], T{4});

	vector[0] = T{5};
	vector[1] = T{6};
	vector[2] = T{7};
	vector[3] = T{8};

	EXPECT_EQ(vector, (Vec4<T>{T{5}, T{6}, T{7}, T{8}}));
}

template <typename T>
void testVec4Arithmetic() {
	const Vec4<T> kLhs{T{4}, T{8}, T{12}, T{16}};
	const Vec4<T> kRhs{T{2}, T{4}, T{6}, T{8}};

	EXPECT_EQ(+kLhs, kLhs);
	EXPECT_EQ(-kLhs, (Vec4<T>{T{-4}, T{-8}, T{-12}, T{-16}}));

	EXPECT_EQ(kLhs + kRhs, (Vec4<T>{T{6}, T{12}, T{18}, T{24}}));
	EXPECT_EQ(kLhs - kRhs, (Vec4<T>{T{2}, T{4}, T{6}, T{8}}));
	EXPECT_EQ(kLhs * kRhs, (Vec4<T>{T{8}, T{32}, T{72}, T{128}}));
	EXPECT_EQ(kLhs / kRhs, (Vec4<T>{T{2}, T{2}, T{2}, T{2}}));

	EXPECT_EQ(kLhs * T{2}, (Vec4<T>{T{8}, T{16}, T{24}, T{32}}));
	EXPECT_EQ(T{2} * kLhs, (Vec4<T>{T{8}, T{16}, T{24}, T{32}}));
	EXPECT_EQ(kLhs / T{2}, (Vec4<T>{T{2}, T{4}, T{6}, T{8}}));

	Vec4<T> value = kLhs;

	EXPECT_EQ(&(value += kRhs), &value);
	EXPECT_EQ(&(value -= kRhs), &value);
	EXPECT_EQ(&(value *= kRhs), &value);
	EXPECT_EQ(&(value /= kRhs), &value);
	EXPECT_EQ(&(value *= T{2}), &value);
	EXPECT_EQ(&(value /= T{2}), &value);

	EXPECT_EQ(value, kLhs);
}

template <typename T>
void testVec4Geometry() {
	const Vec4<T> kVector{T{3}, T{4}, T{0}, T{0}};
	const Vec4<T> kOther{T{2}, T{1}, T{3}, T{4}};

	EXPECT_EQ(kVector.dot(kOther), T{10});
	EXPECT_EQ(dot(kVector, kOther), T{10});

	EXPECT_EQ(kVector.lengthSquared(), T{25});
	EXPECT_NEAR(kVector.length(), T{5}, epsilon<T>());
	EXPECT_NEAR(kVector.robustLength(), T{5}, epsilon<T>());

	expectNear(kVector.normalised(), (Vec4<T>{T{0.6}, T{0.8}, T{0}, T{0}}));
	EXPECT_EQ(Vec4<T>{}.normalisedOrZero(), Vec4<T>{});

	const Vec4<T> kStart{T{0}, T{2}, T{4}, T{6}};
	const Vec4<T> kEnd{T{4}, T{6}, T{8}, T{10}};

	EXPECT_EQ(kStart.lerp(kEnd, T{0.5}), (Vec4<T>{T{2}, T{4}, T{6}, T{8}}));
	EXPECT_EQ(lerp(kStart, kEnd, T{0.5}), (Vec4<T>{T{2}, T{4}, T{6}, T{8}}));

	const Vec4<T> kClose{T{1}, T{2}, T{3}, T{4}};
	const Vec4<T> kNearlyClose{T{1} + (epsilon<T>() / T{2}), T{2} - (epsilon<T>() / T{2}), T{3}, T{4}};

	EXPECT_TRUE(kClose.nearlyEqual(kNearlyClose, epsilon<T>()));
	EXPECT_TRUE(nearlyEqual(kClose, kNearlyClose, epsilon<T>()));
	EXPECT_FALSE(kClose.nearlyEqual(Vec4<T>{T{2}, T{3}, T{4}, T{5}}, epsilon<T>()));
}

TEST(Vec4Test, SupportsBasicAccess) {
	testVec4Basics<int>();
	testVec4Basics<float>();
	testVec4Basics<double>();
}

TEST(Vec4Test, SupportsArithmetic) {
	testVec4Arithmetic<int>();
	testVec4Arithmetic<float>();
	testVec4Arithmetic<double>();
}

TEST(Vec4Test, SupportsGeometry) {
	testVec4Geometry<float>();
	testVec4Geometry<double>();
}

static_assert(sizeof(Vec2<int>) == sizeof(int) * 2);
static_assert(sizeof(Vec2<float>) == sizeof(float) * 2);
static_assert(sizeof(Vec2<double>) == sizeof(double) * 2);

static_assert(sizeof(Vec3<int>) == sizeof(int) * 3);
static_assert(sizeof(Vec3<float>) == sizeof(float) * 3);
static_assert(sizeof(Vec3<double>) == sizeof(double) * 3);

static_assert(sizeof(Vec4<int>) == sizeof(int) * 4);
static_assert(sizeof(Vec4<float>) == sizeof(float) * 4);
static_assert(sizeof(Vec4<double>) == sizeof(double) * 4);

static_assert(alignof(Vec2<float>) == alignof(float));
static_assert(alignof(Vec3<float>) == alignof(float));
static_assert(alignof(Vec4<float>) == alignof(float));

static_assert(alignof(Vec2<double>) == alignof(double));
static_assert(alignof(Vec3<double>) == alignof(double));
static_assert(alignof(Vec4<double>) == alignof(double));

static_assert(std::is_trivially_copyable_v<Vec2<float>>);
static_assert(std::is_trivially_copyable_v<Vec3<float>>);
static_assert(std::is_trivially_copyable_v<Vec4<float>>);

static_assert(std::is_standard_layout_v<Vec2<float>>);
static_assert(std::is_standard_layout_v<Vec3<float>>);
static_assert(std::is_standard_layout_v<Vec4<float>>);

} // namespace

} // namespace trivial::math

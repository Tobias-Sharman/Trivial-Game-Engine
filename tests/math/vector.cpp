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
	Vec2<T> lhs{T{4}, T{8}};
	Vec2<T> rhs{T{2}, T{4}};

	EXPECT_EQ(+lhs, lhs);
	EXPECT_EQ(-lhs, (Vec2<T>{T{-4}, T{-8}}));

	EXPECT_EQ(lhs + rhs, (Vec2<T>{T{6}, T{12}}));
	EXPECT_EQ(lhs - rhs, (Vec2<T>{T{2}, T{4}}));
	EXPECT_EQ(lhs * rhs, (Vec2<T>{T{8}, T{32}}));
	EXPECT_EQ(lhs / rhs, (Vec2<T>{T{2}, T{2}}));

	EXPECT_EQ(lhs * T{2}, (Vec2<T>{T{8}, T{16}}));
	EXPECT_EQ(T{2} * lhs, (Vec2<T>{T{8}, T{16}}));
	EXPECT_EQ(lhs / T{2}, (Vec2<T>{T{2}, T{4}}));

	Vec2<T> value = lhs;

	EXPECT_EQ(&(value += rhs), &value);
	EXPECT_EQ(value, (Vec2<T>{T{6}, T{12}}));

	EXPECT_EQ(&(value -= rhs), &value);
	EXPECT_EQ(value, lhs);

	EXPECT_EQ(&(value *= rhs), &value);
	EXPECT_EQ(value, (Vec2<T>{T{8}, T{32}}));

	EXPECT_EQ(&(value /= rhs), &value);
	EXPECT_EQ(value, lhs);

	EXPECT_EQ(&(value *= T{2}), &value);
	EXPECT_EQ(value, (Vec2<T>{T{8}, T{16}}));

	EXPECT_EQ(&(value /= T{2}), &value);
	EXPECT_EQ(value, lhs);
}

template <typename T>
void testVec2Geometry() {
	Vec2<T> vector{T{3}, T{4}};
	Vec2<T> other{T{2}, T{1}};

	EXPECT_EQ(vector.dot(other), T{10});
	EXPECT_EQ(dot(vector, other), T{10});

	EXPECT_EQ(vector.cross(other), T{-5});
	EXPECT_EQ(cross(vector, other), T{-5});

	EXPECT_EQ(vector.lengthSquared(), T{25});
	EXPECT_NEAR(vector.length(), T{5}, epsilon<T>());
	EXPECT_NEAR(vector.robustLength(), T{5}, epsilon<T>());

	expectNear(vector.normalised(), (Vec2<T>{T{0.6}, T{0.8}}));
	EXPECT_EQ(Vec2<T>{}.normalisedOrZero(), Vec2<T>{});

	EXPECT_EQ(vector.perpLeft(), (Vec2<T>{T{-4}, T{3}}));
	EXPECT_EQ(vector.perpRight(), (Vec2<T>{T{4}, T{-3}}));

	Vec2<T> start{T{0}, T{2}};
	Vec2<T> end{T{4}, T{6}};

	EXPECT_EQ(start.lerp(end, T{0.5}), (Vec2<T>{T{2}, T{4}}));
	EXPECT_EQ(lerp(start, end, T{0.5}), (Vec2<T>{T{2}, T{4}}));

	Vec2<T> close{T{1}, T{2}};
	Vec2<T> nearlyClose{T{1} + (epsilon<T>() / T{2}), T{2} - (epsilon<T>() / T{2})};

	EXPECT_TRUE(close.nearlyEqual(nearlyClose, epsilon<T>()));
	EXPECT_TRUE(nearlyEqual(close, nearlyClose, epsilon<T>()));
	EXPECT_FALSE(close.nearlyEqual(Vec2<T>{T{2}, T{3}}, epsilon<T>()));
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
	Vec3<T> lhs{T{4}, T{8}, T{12}};
	Vec3<T> rhs{T{2}, T{4}, T{6}};

	EXPECT_EQ(+lhs, lhs);
	EXPECT_EQ(-lhs, (Vec3<T>{T{-4}, T{-8}, T{-12}}));

	EXPECT_EQ(lhs + rhs, (Vec3<T>{T{6}, T{12}, T{18}}));
	EXPECT_EQ(lhs - rhs, (Vec3<T>{T{2}, T{4}, T{6}}));
	EXPECT_EQ(lhs * rhs, (Vec3<T>{T{8}, T{32}, T{72}}));
	EXPECT_EQ(lhs / rhs, (Vec3<T>{T{2}, T{2}, T{2}}));

	EXPECT_EQ(lhs * T{2}, (Vec3<T>{T{8}, T{16}, T{24}}));
	EXPECT_EQ(T{2} * lhs, (Vec3<T>{T{8}, T{16}, T{24}}));
	EXPECT_EQ(lhs / T{2}, (Vec3<T>{T{2}, T{4}, T{6}}));

	Vec3<T> value = lhs;

	EXPECT_EQ(&(value += rhs), &value);
	EXPECT_EQ(&(value -= rhs), &value);
	EXPECT_EQ(&(value *= rhs), &value);
	EXPECT_EQ(&(value /= rhs), &value);
	EXPECT_EQ(&(value *= T{2}), &value);
	EXPECT_EQ(&(value /= T{2}), &value);

	EXPECT_EQ(value, lhs);
}

template <typename T>
void testVec3Geometry() {
	Vec3<T> vector{T{3}, T{4}, T{0}};
	Vec3<T> other{T{2}, T{1}, T{3}};

	EXPECT_EQ(vector.dot(other), T{10});
	EXPECT_EQ(dot(vector, other), T{10});

	EXPECT_EQ(vector.cross(other), (Vec3<T>{T{12}, T{-9}, T{-5}}));
	EXPECT_EQ(cross(vector, other), (Vec3<T>{T{12}, T{-9}, T{-5}}));

	EXPECT_EQ(vector.lengthSquared(), T{25});
	EXPECT_NEAR(vector.length(), T{5}, epsilon<T>());
	EXPECT_NEAR(vector.robustLength(), T{5}, epsilon<T>());

	expectNear(vector.normalised(), (Vec3<T>{T{0.6}, T{0.8}, T{0}}));
	EXPECT_EQ(Vec3<T>{}.normalisedOrZero(), Vec3<T>{});

	Vec3<T> start{T{0}, T{2}, T{4}};
	Vec3<T> end{T{4}, T{6}, T{8}};

	EXPECT_EQ(start.lerp(end, T{0.5}), (Vec3<T>{T{2}, T{4}, T{6}}));
	EXPECT_EQ(lerp(start, end, T{0.5}), (Vec3<T>{T{2}, T{4}, T{6}}));

	Vec3<T> close{T{1}, T{2}, T{3}};
	Vec3<T> nearlyClose{T{1} + (epsilon<T>() / T{2}), T{2} - (epsilon<T>() / T{2}), T{3}};

	EXPECT_TRUE(close.nearlyEqual(nearlyClose, epsilon<T>()));
	EXPECT_TRUE(nearlyEqual(close, nearlyClose, epsilon<T>()));
	EXPECT_FALSE(close.nearlyEqual(Vec3<T>{T{2}, T{3}, T{4}}, epsilon<T>()));
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
	Vec4<T> lhs{T{4}, T{8}, T{12}, T{16}};
	Vec4<T> rhs{T{2}, T{4}, T{6}, T{8}};

	EXPECT_EQ(+lhs, lhs);
	EXPECT_EQ(-lhs, (Vec4<T>{T{-4}, T{-8}, T{-12}, T{-16}}));

	EXPECT_EQ(lhs + rhs, (Vec4<T>{T{6}, T{12}, T{18}, T{24}}));
	EXPECT_EQ(lhs - rhs, (Vec4<T>{T{2}, T{4}, T{6}, T{8}}));
	EXPECT_EQ(lhs * rhs, (Vec4<T>{T{8}, T{32}, T{72}, T{128}}));
	EXPECT_EQ(lhs / rhs, (Vec4<T>{T{2}, T{2}, T{2}, T{2}}));

	EXPECT_EQ(lhs * T{2}, (Vec4<T>{T{8}, T{16}, T{24}, T{32}}));
	EXPECT_EQ(T{2} * lhs, (Vec4<T>{T{8}, T{16}, T{24}, T{32}}));
	EXPECT_EQ(lhs / T{2}, (Vec4<T>{T{2}, T{4}, T{6}, T{8}}));

	Vec4<T> value = lhs;

	EXPECT_EQ(&(value += rhs), &value);
	EXPECT_EQ(&(value -= rhs), &value);
	EXPECT_EQ(&(value *= rhs), &value);
	EXPECT_EQ(&(value /= rhs), &value);
	EXPECT_EQ(&(value *= T{2}), &value);
	EXPECT_EQ(&(value /= T{2}), &value);

	EXPECT_EQ(value, lhs);
}

template <typename T>
void testVec4Geometry() {
	Vec4<T> vector{T{3}, T{4}, T{0}, T{0}};
	Vec4<T> other{T{2}, T{1}, T{3}, T{4}};

	EXPECT_EQ(vector.dot(other), T{10});
	EXPECT_EQ(dot(vector, other), T{10});

	EXPECT_EQ(vector.lengthSquared(), T{25});
	EXPECT_NEAR(vector.length(), T{5}, epsilon<T>());
	EXPECT_NEAR(vector.robustLength(), T{5}, epsilon<T>());

	expectNear(vector.normalised(), (Vec4<T>{T{0.6}, T{0.8}, T{0}, T{0}}));
	EXPECT_EQ(Vec4<T>{}.normalisedOrZero(), Vec4<T>{});

	Vec4<T> start{T{0}, T{2}, T{4}, T{6}};
	Vec4<T> end{T{4}, T{6}, T{8}, T{10}};

	EXPECT_EQ(start.lerp(end, T{0.5}), (Vec4<T>{T{2}, T{4}, T{6}, T{8}}));
	EXPECT_EQ(lerp(start, end, T{0.5}), (Vec4<T>{T{2}, T{4}, T{6}, T{8}}));

	Vec4<T> close{T{1}, T{2}, T{3}, T{4}};
	Vec4<T> nearlyClose{T{1} + (epsilon<T>() / T{2}), T{2} - (epsilon<T>() / T{2}), T{3}, T{4}};

	EXPECT_TRUE(close.nearlyEqual(nearlyClose, epsilon<T>()));
	EXPECT_TRUE(nearlyEqual(close, nearlyClose, epsilon<T>()));
	EXPECT_FALSE(close.nearlyEqual(Vec4<T>{T{2}, T{3}, T{4}, T{5}}, epsilon<T>()));
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

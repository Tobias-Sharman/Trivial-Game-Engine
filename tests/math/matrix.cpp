#include <gtest/gtest.h>

#include <trivial/core/config.h>
#include <trivial/core/math/mat4.h>

namespace trivial::math {

namespace {

// TODO: Add more tests/checks for all maths classes

static_assert(Mat4f::identity() * Vec4f{1.0f, 2.0f, 3.0f, 4.0f} == Vec4f{1.0f, 2.0f, 3.0f, 4.0f});

#if TRIVIAL_ENABLE_SIMD && (defined(__aarch64__) || defined(_M_ARM64) || defined(__x86_64__) || defined(_M_X64))

TEST(Mat4Simd, MatchesScalar) {
	const Mat4f
	    kMatrix{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f};

	const Vec4f kVector{1.0f, 2.0f, 3.0f, 4.0f};

	const Vec4f kRegular = detail::matVecScalar(kMatrix, kVector);
	const Vec4f kSIMD = detail::matVecSimd(kMatrix, kVector);

	EXPECT_NEAR(kSIMD.x, kRegular.x, 1e-5f);
	EXPECT_NEAR(kSIMD.y, kRegular.y, 1e-5f);
	EXPECT_NEAR(kSIMD.z, kRegular.z, 1e-5f);
	EXPECT_NEAR(kSIMD.w, kRegular.w, 1e-5f);
}

#endif

} // namespace

} // namespace trivial::math

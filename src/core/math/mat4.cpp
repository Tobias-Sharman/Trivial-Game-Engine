#include <trivial/core/math/mat4.h>

#if TRIVIAL_ENABLE_SIMD

#if defined(__aarch64__) || defined(_M_ARM64)

#include <arm_neon.h>

namespace trivial::math::detail {

Vec4<float> matVecSimd(const Mat4<float>& m, Vec4<float> v) noexcept {
	const float32x4_t kC0 = vld1q_f32(&m.col0.x);
	const float32x4_t kC1 = vld1q_f32(&m.col1.x);
	const float32x4_t kC2 = vld1q_f32(&m.col2.x);
	const float32x4_t kC3 = vld1q_f32(&m.col3.x);

	float32x4_t result = vmulq_n_f32(kC0, v.x);
	result = vfmaq_n_f32(result, kC1, v.y);
	result = vfmaq_n_f32(result, kC2, v.z);
	result = vfmaq_n_f32(result, kC3, v.w);

	Vec4<float> output;
	vst1q_f32(&output.x, result);
	return output;
}

} // namespace trivial::math::detail

#elif defined(__x86_64__) || defined(_M_X64)

// TOTEST: Untested branch
// TODO: Add run time dispatch for FMA, do later when caring about windows support for J

#include <immintrin.h>

namespace trivial::math::detail {

Vec4<float> matVecSimd(const Mat4<float>& m, Vec4<float> v) noexcept {
	const __m128 kC0 = _mm_load_ps(&m.col0.x);
	const __m128 kC1 = _mm_load_ps(&m.col1.x);
	const __m128 kC2 = _mm_load_ps(&m.col2.x);
	const __m128 kC3 = _mm_load_ps(&m.col3.x);

	__m128 result = _mm_mul_ps(kC0, _mm_set1_ps(v.x));
	result = _mm_add_ps(result, _mm_mul_ps(kC1, _mm_set1_ps(v.y)));
	result = _mm_add_ps(result, _mm_mul_ps(kC2, _mm_set1_ps(v.z)));
	result = _mm_add_ps(result, _mm_mul_ps(kC3, _mm_set1_ps(v.w)));

	Vec4<float> output;
	_mm_store_ps(&output.x, result);
	return output;
}

} // namespace trivial::math::detail

#endif // Architecture type

#endif // TRIVIAL_ENABLE_SIMD

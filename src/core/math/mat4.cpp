#include <trivial/core/math/mat4.h>

#include <trivial/core/math/math_config.h>
#include <trivial/core/platform.h>

#if !TRIVIAL_MATH_DISABLE_SIMD_BACKEND
#if TRIVIAL_ARCH_X86_64
#include <immintrin.h>

#elif TRIVIAL_ARCH_ARM64
#include <arm_neon.h>

#endif // Intrinsic header dispatch

#endif // SIMD check

namespace trivial::math::detail {

Vec4f multiplyMat4Vec4(const Mat4f& matrix, const Vec4f& vector) noexcept {
#if TRIVIAL_MATH_DISABLE_SIMD_BACKEND
	return (matrix.col0 * vector.x + matrix.col1 * vector.y) + (matrix.col2 * vector.z + matrix.col3 * vector.w);
	// Weird braces to match non-FMA SIMD style

#elif TRIVIAL_ARCH_X86_64
	const __m128 kC0 = _mm_loadu_ps(&matrix.col0.x);
	const __m128 kC1 = _mm_loadu_ps(&matrix.col1.x);
	const __m128 kC2 = _mm_loadu_ps(&matrix.col2.x);
	const __m128 kC3 = _mm_loadu_ps(&matrix.col3.x);
	const __m128 kInput = _mm_loadu_ps(&vector.x);

	const __m128 kX = _mm_shuffle_ps(kInput, kInput, _MM_SHUFFLE(0, 0, 0, 0));
	const __m128 kY = _mm_shuffle_ps(kInput, kInput, _MM_SHUFFLE(1, 1, 1, 1));
	const __m128 kZ = _mm_shuffle_ps(kInput, kInput, _MM_SHUFFLE(2, 2, 2, 2));
	const __m128 kW = _mm_shuffle_ps(kInput, kInput, _MM_SHUFFLE(3, 3, 3, 3));

	const __m128 kXY = _mm_add_ps(_mm_mul_ps(kC0, kX), _mm_mul_ps(kC1, kY));
	const __m128 kZW = _mm_add_ps(_mm_mul_ps(kC2, kZ), _mm_mul_ps(kC3, kW));
	const __m128 kResult = _mm_add_ps(kXY, kZW);

	Vec4f output;
	_mm_storeu_ps(&output.x, kResult);
	return output;

#elif TRIVIAL_ARCH_ARM64
	const float32x4_t kC0 = vld1q_f32(&matrix.col0.x);
	const float32x4_t kC1 = vld1q_f32(&matrix.col1.x);
	const float32x4_t kC2 = vld1q_f32(&matrix.col2.x);
	const float32x4_t kC3 = vld1q_f32(&matrix.col3.x);
	const float32x4_t kInput = vld1q_f32(&vector.x);

#if TRIVIAL_MATH_DISABLE_FMA
	const float32x4_t kXY = vaddq_f32(vmulq_laneq_f32(kC0, kInput, 0), vmulq_laneq_f32(kC1, kInput, 1));
	const float32x4_t kZW = vaddq_f32(vmulq_laneq_f32(kC2, kInput, 2), vmulq_laneq_f32(kC3, kInput, 3));
	const float32x4_t kResult = vaddq_f32(kXY, kZW);

	Vec4f output;
	vst1q_f32(&output.x, kResult);
	return output;

#else
	float32x4_t result = vmulq_laneq_f32(kC0, kInput, 0);
	result = vfmaq_laneq_f32(result, kC1, kInput, 1);
	result = vfmaq_laneq_f32(result, kC2, kInput, 2);
	result = vfmaq_laneq_f32(result, kC3, kInput, 3);

	Vec4f output;
	vst1q_f32(&output.x, result);
	return output;
#endif // FMA check

#endif // CPU architecture and SIMD dispatch
}

Mat4f multiplyMat4Mat4(const Mat4f& lhs, const Mat4f& rhs) noexcept {
#if TRIVIAL_MATH_DISABLE_SIMD_BACKEND
	return {(lhs.col0 * rhs.col0.x + lhs.col1 * rhs.col0.y) + (lhs.col2 * rhs.col0.z + lhs.col3 * rhs.col0.w),
	        (lhs.col0 * rhs.col1.x + lhs.col1 * rhs.col1.y) + (lhs.col2 * rhs.col1.z + lhs.col3 * rhs.col1.w),
	        (lhs.col0 * rhs.col2.x + lhs.col1 * rhs.col2.y) + (lhs.col2 * rhs.col2.z + lhs.col3 * rhs.col2.w),
	        (lhs.col0 * rhs.col3.x + lhs.col1 * rhs.col3.y) + (lhs.col2 * rhs.col3.z + lhs.col3 * rhs.col3.w)};
	// Weird braces to match non-FMA SIMD style

#elif TRIVIAL_ARCH_X86_64
	const __m128 kL0 = _mm_loadu_ps(&lhs.col0.x);
	const __m128 kL1 = _mm_loadu_ps(&lhs.col1.x);
	const __m128 kL2 = _mm_loadu_ps(&lhs.col2.x);
	const __m128 kL3 = _mm_loadu_ps(&lhs.col3.x);

	Mat4f output;
	{
		const __m128 kR = _mm_loadu_ps(&rhs.col0.x);

		const __m128 kX = _mm_shuffle_ps(kR, kR, _MM_SHUFFLE(0, 0, 0, 0));
		const __m128 kY = _mm_shuffle_ps(kR, kR, _MM_SHUFFLE(1, 1, 1, 1));
		const __m128 kZ = _mm_shuffle_ps(kR, kR, _MM_SHUFFLE(2, 2, 2, 2));
		const __m128 kW = _mm_shuffle_ps(kR, kR, _MM_SHUFFLE(3, 3, 3, 3));

		const __m128 kXY = _mm_add_ps(_mm_mul_ps(kL0, kX), _mm_mul_ps(kL1, kY));
		const __m128 kZW = _mm_add_ps(_mm_mul_ps(kL2, kZ), _mm_mul_ps(kL3, kW));

		const __m128 kResult = _mm_add_ps(kXY, kZW);
		_mm_storeu_ps(&output.col0.x, kResult);
	}
	{
		const __m128 kR = _mm_loadu_ps(&rhs.col1.x);

		const __m128 kX = _mm_shuffle_ps(kR, kR, _MM_SHUFFLE(0, 0, 0, 0));
		const __m128 kY = _mm_shuffle_ps(kR, kR, _MM_SHUFFLE(1, 1, 1, 1));
		const __m128 kZ = _mm_shuffle_ps(kR, kR, _MM_SHUFFLE(2, 2, 2, 2));
		const __m128 kW = _mm_shuffle_ps(kR, kR, _MM_SHUFFLE(3, 3, 3, 3));

		const __m128 kXY = _mm_add_ps(_mm_mul_ps(kL0, kX), _mm_mul_ps(kL1, kY));
		const __m128 kZW = _mm_add_ps(_mm_mul_ps(kL2, kZ), _mm_mul_ps(kL3, kW));

		const __m128 kResult = _mm_add_ps(kXY, kZW);
		_mm_storeu_ps(&output.col1.x, kResult);
	}
	{
		const __m128 kR = _mm_loadu_ps(&rhs.col2.x);

		const __m128 kX = _mm_shuffle_ps(kR, kR, _MM_SHUFFLE(0, 0, 0, 0));
		const __m128 kY = _mm_shuffle_ps(kR, kR, _MM_SHUFFLE(1, 1, 1, 1));
		const __m128 kZ = _mm_shuffle_ps(kR, kR, _MM_SHUFFLE(2, 2, 2, 2));
		const __m128 kW = _mm_shuffle_ps(kR, kR, _MM_SHUFFLE(3, 3, 3, 3));

		const __m128 kXY = _mm_add_ps(_mm_mul_ps(kL0, kX), _mm_mul_ps(kL1, kY));
		const __m128 kZW = _mm_add_ps(_mm_mul_ps(kL2, kZ), _mm_mul_ps(kL3, kW));

		const __m128 kResult = _mm_add_ps(kXY, kZW);
		_mm_storeu_ps(&output.col2.x, kResult);
	}
	{
		const __m128 kR = _mm_loadu_ps(&rhs.col3.x);

		const __m128 kX = _mm_shuffle_ps(kR, kR, _MM_SHUFFLE(0, 0, 0, 0));
		const __m128 kY = _mm_shuffle_ps(kR, kR, _MM_SHUFFLE(1, 1, 1, 1));
		const __m128 kZ = _mm_shuffle_ps(kR, kR, _MM_SHUFFLE(2, 2, 2, 2));
		const __m128 kW = _mm_shuffle_ps(kR, kR, _MM_SHUFFLE(3, 3, 3, 3));

		const __m128 kXY = _mm_add_ps(_mm_mul_ps(kL0, kX), _mm_mul_ps(kL1, kY));
		const __m128 kZW = _mm_add_ps(_mm_mul_ps(kL2, kZ), _mm_mul_ps(kL3, kW));

		const __m128 kResult = _mm_add_ps(kXY, kZW);
		_mm_storeu_ps(&output.col3.x, kResult);
	}
	return output;

#elif TRIVIAL_ARCH_ARM64
	const float32x4_t kL0 = vld1q_f32(&lhs.col0.x);
	const float32x4_t kL1 = vld1q_f32(&lhs.col1.x);
	const float32x4_t kL2 = vld1q_f32(&lhs.col2.x);
	const float32x4_t kL3 = vld1q_f32(&lhs.col3.x);

	Mat4f output;

#if TRIVIAL_MATH_DISABLE_FMA
	{
		const float32x4_t kR = vld1q_f32(&rhs.col0.x);

		const float32x4_t kXY = vaddq_f32(vmulq_laneq_f32(kL0, kR, 0), vmulq_laneq_f32(kL1, kR, 1));
		const float32x4_t kZW = vaddq_f32(vmulq_laneq_f32(kL2, kR, 2), vmulq_laneq_f32(kL3, kR, 3));

		const float32x4_t kResult = vaddq_f32(kXY, kZW);
		vst1q_f32(&output.col0.x, kResult);
	}
	{
		const float32x4_t kR = vld1q_f32(&rhs.col1.x);

		const float32x4_t kXY = vaddq_f32(vmulq_laneq_f32(kL0, kR, 0), vmulq_laneq_f32(kL1, kR, 1));
		const float32x4_t kZW = vaddq_f32(vmulq_laneq_f32(kL2, kR, 2), vmulq_laneq_f32(kL3, kR, 3));

		const float32x4_t kResult = vaddq_f32(kXY, kZW);
		vst1q_f32(&output.col1.x, kResult);
	}
	{
		const float32x4_t kR = vld1q_f32(&rhs.col2.x);

		const float32x4_t kXY = vaddq_f32(vmulq_laneq_f32(kL0, kR, 0), vmulq_laneq_f32(kL1, kR, 1));
		const float32x4_t kZW = vaddq_f32(vmulq_laneq_f32(kL2, kR, 2), vmulq_laneq_f32(kL3, kR, 3));

		const float32x4_t kResult = vaddq_f32(kXY, kZW);
		vst1q_f32(&output.col2.x, kResult);
	}
	{
		const float32x4_t kR = vld1q_f32(&rhs.col3.x);

		const float32x4_t kXY = vaddq_f32(vmulq_laneq_f32(kL0, kR, 0), vmulq_laneq_f32(kL1, kR, 1));
		const float32x4_t kZW = vaddq_f32(vmulq_laneq_f32(kL2, kR, 2), vmulq_laneq_f32(kL3, kR, 3));

		const float32x4_t kResult = vaddq_f32(kXY, kZW);
		vst1q_f32(&output.col3.x, kResult);
	}
#else
	{
		const float32x4_t kR = vld1q_f32(&rhs.col0.x);
		float32x4_t result = vmulq_laneq_f32(kL0, kR, 0);

		result = vfmaq_laneq_f32(result, kL1, kR, 1);
		result = vfmaq_laneq_f32(result, kL2, kR, 2);
		result = vfmaq_laneq_f32(result, kL3, kR, 3);

		vst1q_f32(&output.col0.x, result);
	}
	{
		const float32x4_t kR = vld1q_f32(&rhs.col1.x);
		float32x4_t result = vmulq_laneq_f32(kL0, kR, 0);

		result = vfmaq_laneq_f32(result, kL1, kR, 1);
		result = vfmaq_laneq_f32(result, kL2, kR, 2);
		result = vfmaq_laneq_f32(result, kL3, kR, 3);

		vst1q_f32(&output.col1.x, result);
	}
	{
		const float32x4_t kR = vld1q_f32(&rhs.col2.x);
		float32x4_t result = vmulq_laneq_f32(kL0, kR, 0);

		result = vfmaq_laneq_f32(result, kL1, kR, 1);
		result = vfmaq_laneq_f32(result, kL2, kR, 2);
		result = vfmaq_laneq_f32(result, kL3, kR, 3);

		vst1q_f32(&output.col2.x, result);
	}
	{
		const float32x4_t kR = vld1q_f32(&rhs.col3.x);
		float32x4_t result = vmulq_laneq_f32(kL0, kR, 0);

		result = vfmaq_laneq_f32(result, kL1, kR, 1);
		result = vfmaq_laneq_f32(result, kL2, kR, 2);
		result = vfmaq_laneq_f32(result, kL3, kR, 3);

		vst1q_f32(&output.col3.x, result);
	}
#endif // FMA check
	return output;

#endif // CPU architecture and SIMD dispatch
}

} // namespace trivial::math::detail

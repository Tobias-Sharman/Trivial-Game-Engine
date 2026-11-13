#ifndef TRIVIAL_CORE_MATH_UINT128_H
#define TRIVIAL_CORE_MATH_UINT128_H

#include <cstddef>
#include <cstdint>

#include <trivial/core/bits/bits_constants.h>
#include <trivial/core/compiler.h>

#if TRIVIAL_COMPILER_MSVC
#include <intrin.h>
#include <limits>

#include <trivial/core/platform.h>
#endif // TRIVIAL_COMPILER_MSVC

namespace trivial::math {

struct UInt128 {
	std::uint64_t low = 0;
	std::uint64_t high = 0;
};

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr UInt128 multiplyFull64(std::uint64_t left, std::uint64_t right) noexcept {
#if TRIVIAL_COMPILER_MSVC
	if !consteval {
#if TRIVIAL_ARCH_X86_64
		UInt128 result;
		result.low = _umul128(left, right, &result.high);
		return result;
#else
		return {
		    .low = left * right,
		    .high = __umulh(left, right),
		};
#endif // TRIVIAL_ARCH_X86_64
	}

	constexpr std::uint64_t kLowMask = std::numeric_limits<std::uint32_t>::max();
	constexpr std::size_t kHalfBits = TRIVIAL_BITS_BITS_IN(std::uint32_t);

	const std::uint64_t kLeftLow = left & kLowMask;
	const std::uint64_t kLeftHigh = left >> kHalfBits;
	const std::uint64_t kRightLow = right & kLowMask;
	const std::uint64_t kRightHigh = right >> kHalfBits;

	const std::uint64_t kLowLow = kLeftLow * kRightLow;
	const std::uint64_t kLowHigh = kLeftLow * kRightHigh;
	const std::uint64_t kHighLow = kLeftHigh * kRightLow;
	const std::uint64_t kHighHigh = kLeftHigh * kRightHigh;

	const std::uint64_t kMiddle = (kLowLow >> kHalfBits) + (kLowHigh & kLowMask) + (kHighLow & kLowMask);

	return {
	    .low = (kMiddle << kHalfBits) | (kLowLow & kLowMask),
	    .high = kHighHigh + (kLowHigh >> kHalfBits) + (kHighLow >> kHalfBits) + (kMiddle >> kHalfBits),
	};
#else
	__extension__ const unsigned __int128 kProduct = static_cast<unsigned __int128>(left) * right;

	return {
	    .low = static_cast<std::uint64_t>(kProduct),
	    .high = static_cast<std::uint64_t>(kProduct >> TRIVIAL_BITS_BITS_IN(std::uint64_t)),
	};
#endif // TRIVIAL_COMPILER_MSVC
}

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr std::uint64_t multiplyHigh64(std::uint64_t left,
                                                                          std::uint64_t right) noexcept {
	return multiplyFull64(left, right).high;
}

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr std::uint64_t multiplyHigh64By32(std::uint64_t left,
                                                                              std::uint32_t right) noexcept {
#if TRIVIAL_COMPILER_MSVC
	if !consteval {
		return __umulh(left, right);
	}

	constexpr std::size_t kHalfBits = TRIVIAL_BITS_BITS_IN(std::uint32_t);

	const std::uint64_t kRight = right;

	return ((kRight * (left >> kHalfBits)) + ((kRight * static_cast<std::uint32_t>(left)) >> kHalfBits)) >> kHalfBits;
#else
	__extension__ const unsigned __int128 kProduct = static_cast<unsigned __int128>(left) * right;

	return static_cast<std::uint64_t>(kProduct >> TRIVIAL_BITS_BITS_IN(std::uint64_t));
#endif // TRIVIAL_COMPILER_MSVC
}

} // namespace trivial::math

#endif // TRIVIAL_CORE_MATH_UINT128_H

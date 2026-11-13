#ifndef TRIVIAL_CORE_MATH_SCALAR_H
#define TRIVIAL_CORE_MATH_SCALAR_H

#include <cmath>

#include <trivial/core/assert.h>
#include <trivial/core/compiler.h>
#include <trivial/core/math/concepts.h>
#include <trivial/core/platform.h>

namespace trivial::math {

template <Arithmetic T>
[[nodiscard]] constexpr T min(T left, T right) noexcept {
	if constexpr (FloatingPoint<T>) {
		TRIVIAL_ASSERT(!std::isnan(left) && !std::isnan(right));
	}

	return left < right ? left : right;
}

template <Arithmetic T>
[[nodiscard]] constexpr T max(T left, T right) noexcept {
	if constexpr (FloatingPoint<T>) {
		TRIVIAL_ASSERT(!std::isnan(left) && !std::isnan(right));
	}

	return left < right ? right : left;
}

template <Arithmetic T>
[[nodiscard]] constexpr T clamp(T value, T low, T high) noexcept {
	TRIVIAL_ASSERT(!(high < low));

	return min(max(value, low), high);
}

template <FloatingPoint T>
[[nodiscard]] constexpr T fastMin(T left, T right) noexcept {
	TRIVIAL_ASSERT(!std::isnan(left) && !std::isnan(right));

	if constexpr (TRIVIAL_ARCH_ARM64 && !TRIVIAL_COMPILER_MSVC) {
		if !consteval {
			return std::fmin(left, right);
		}
	}

	return left < right ? left : right;
}

template <FloatingPoint T>
[[nodiscard]] constexpr T fastMax(T left, T right) noexcept {
	TRIVIAL_ASSERT(!std::isnan(left) && !std::isnan(right));

	if constexpr (TRIVIAL_ARCH_ARM64 && !TRIVIAL_COMPILER_MSVC) {
		if !consteval {
			return std::fmax(left, right);
		}
	}

	return left < right ? right : left;
}

template <FloatingPoint T>
[[nodiscard]] constexpr T fastClamp(T value, T low, T high) noexcept {
	TRIVIAL_ASSERT(!(high < low));

	return fastMin(fastMax(value, low), high);
}

} // namespace trivial::math

#endif // TRIVIAL_CORE_MATH_SCALAR_H

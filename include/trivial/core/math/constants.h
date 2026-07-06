#ifndef TRIVIAL_CORE_MATH_CONSTANTS_H
#define TRIVIAL_CORE_MATH_CONSTANTS_H

#include <numbers>

#include <trivial/core/math/concepts.h>

namespace trivial::math::constants {

template <FloatingPoint T>
inline constexpr T g_kPi = std::numbers::pi_v<T>;

template <FloatingPoint T>
inline constexpr T g_kTwoPi = T{2} * g_kPi<T>;

template <FloatingPoint T>
inline constexpr T g_kHalfPi = g_kPi<T> / T{2};

template <FloatingPoint T>
inline constexpr T g_kQuarterPi = g_kPi<T> / T{4};

template <FloatingPoint T>
inline constexpr T g_kInvPi = T{1} / g_kPi<T>;

template <FloatingPoint T>
inline constexpr T g_kDegreesToRadians = g_kPi<T> / T{180};

template <FloatingPoint T>
inline constexpr T g_kRadiansToDegrees = T{180} / g_kPi<T>;

} // namespace trivial::math::constants

#endif // TRIVIAL_CORE_MATH_CONSTANTS_H

#ifndef TRIVIAL_CORE_MATH_CONCEPTS_H
#define TRIVIAL_CORE_MATH_CONCEPTS_H

#include <concepts>
#include <type_traits>

// NOTE: Later will add extension for fixed point types for improved determinism
//       When doing so wrap numeric traits like epsilon, infinity, max etc.

namespace trivial::math {

template <typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

template <typename T>
concept Integral = std::integral<T>;

template <typename T>
concept SignedIntegral = std::signed_integral<T>;

template <typename T>
concept UnsignedIntegral = std::unsigned_integral<T>;

template <typename T>
concept FloatingPoint = std::floating_point<T>;

} // namespace trivial::math

#endif // TRIVIAL_CORE_MATH_CONCEPTS_H

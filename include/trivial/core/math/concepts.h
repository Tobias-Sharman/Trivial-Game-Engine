#ifndef TRIVIAL_CORE_MATH_CONCEPTS_H
#define TRIVIAL_CORE_MATH_CONCEPTS_H

#include <concepts>

namespace trivial::math {

template <typename T>
concept Arithmetic = std::integral<T> || std::floating_point<T>;

} // namespace trivial::math

#endif // TRIVIAL_CORE_MATH_CONCEPTS_H

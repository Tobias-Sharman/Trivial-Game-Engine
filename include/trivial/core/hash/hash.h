#ifndef TRIVIAL_CORE_HASH_HASH_H
#define TRIVIAL_CORE_HASH_HASH_H

#include <cstddef>
#include <cstdint>

#include <trivial/core/compiler.h>
#include <trivial/core/hash/hash_constants.h>

namespace trivial::hash {

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr std::size_t fibonacciHash(const std::uintptr_t kKey,
                                                                       const int kBits) noexcept {
	return kKey * TRIVIAL_HASH_FIBONACCI_MULTIPLIER >> (64 - kBits); // NOLINT(readability-magic-numbers)
}

} // namespace trivial::hash

#endif // TRIVIAL_CORE_HASH_HASH_H

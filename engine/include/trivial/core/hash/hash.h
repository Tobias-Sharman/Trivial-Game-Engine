#ifndef TRIVIAL_CORE_HASH_HASH_H
#define TRIVIAL_CORE_HASH_HASH_H

#include <cstddef>
#include <cstdint>

#include <trivial/core/assert.h>
#include <trivial/core/compiler.h>
#include <trivial/core/hash/hash_constants.h>

namespace trivial::hash {

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr std::size_t fibonacciHash(std::uintptr_t key,
                                                                       std::uint32_t bits) noexcept {
	constexpr std::uint32_t kProductBits = 64U;
	TRIVIAL_ASSERT(bits > 0U && bits <= kProductBits); // NOLINT(readability-simplify-boolean-expr)

	return key * TRIVIAL_HASH_FIBONACCI_MULTIPLIER >> (kProductBits - bits);
}

} // namespace trivial::hash

#endif // TRIVIAL_CORE_HASH_HASH_H

#ifndef TRIVIAL_CORE_HASH_HASH_H
#define TRIVIAL_CORE_HASH_HASH_H

#include <cstddef>
#include <cstdint>

#include <trivial/core/assert.h>
#include <trivial/core/compiler.h>
#include <trivial/core/hash/hash_constants.h>

namespace trivial::hash {

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr std::size_t fibonacciHash(const std::uintptr_t kKey,
                                                                       const std::uint32_t kBits) noexcept {
	constexpr std::uint32_t kProductBits = 64U;
	TRIVIAL_ASSERT(kBits > 0U && kBits <= kProductBits); // NOLINT(readability-simplify-boolean-expr)

	return kKey * TRIVIAL_HASH_FIBONACCI_MULTIPLIER >> (kProductBits - kBits);
}

} // namespace trivial::hash

#endif // TRIVIAL_CORE_HASH_HASH_H

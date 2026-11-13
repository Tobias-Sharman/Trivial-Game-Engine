#ifndef TRIVIAL_CORE_MEMORY_MEMORY_OPS_H
#define TRIVIAL_CORE_MEMORY_MEMORY_OPS_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>

#include <trivial/core/assert.h>
#include <trivial/core/compiler.h>
#include <trivial/core/config.h>
#include <trivial/core/memory/byte_count.h>
#include <trivial/core/memory/pointer_arithmetic.h>

#if TRIVIAL_ENABLE_ASSERTS
#include <trivial/core/memory/alignment.h>
#endif // TRIVIAL_ENABLE_ASSERTS

#define TRIVIAL_MEMORY_OPS_BLOCK_GRANULARITY sizeof(std::size_t)
#define TRIVIAL_MEMORY_OPS_BLOCK_ALIGNMENT (std::size_t{16})
#define TRIVIAL_MEMORY_OPS_ZERO_BLOCK_STORE_BYTES (std::size_t{2} * TRIVIAL_MEMORY_OPS_BLOCK_GRANULARITY)
#define TRIVIAL_MEMORY_OPS_ZERO_BLOCK_FOUR_STORE_LIMIT (std::size_t{4} * TRIVIAL_MEMORY_OPS_ZERO_BLOCK_STORE_BYTES)
#define TRIVIAL_MEMORY_OPS_ZERO_BLOCK_TWO_HALF_LIMIT (std::size_t{8} * TRIVIAL_MEMORY_OPS_ZERO_BLOCK_STORE_BYTES)

namespace trivial::memory {

[[nodiscard]] TRIVIAL_FORCE_INLINE int compare(const void* left, const void* right, ByteCount size) noexcept {
	return std::memcmp(left, right, size.toSize());
}

TRIVIAL_FORCE_INLINE void* copy(void* TRIVIAL_RESTRICT destination,
                                const void* TRIVIAL_RESTRICT source,
                                ByteCount size) noexcept {
	return std::memcpy(destination, source, size.toSize());
}

TRIVIAL_FORCE_INLINE void* move(void* destination, const void* source, ByteCount size) noexcept {
	return std::memmove(destination, source, size.toSize());
}

TRIVIAL_FORCE_INLINE void* fill(void* destination, std::uint8_t pattern, ByteCount size) noexcept {
	return std::memset(destination, pattern, size.toSize());
}

TRIVIAL_FORCE_INLINE void* fillBackward(void* end, std::uint8_t pattern, ByteCount size) noexcept {
	std::memset(pointerMinus(end, size), pattern, size.toSize());
	return end;
}

TRIVIAL_FORCE_INLINE void* zero(void* destination, ByteCount size) noexcept {
	return fill(destination, 0, size);
}

TRIVIAL_FORCE_INLINE void* zeroBackward(void* end, ByteCount size) noexcept {
	return fillBackward(end, 0, size);
}

TRIVIAL_FORCE_INLINE void* copyAligned(void* TRIVIAL_RESTRICT destination,
                                       const void* TRIVIAL_RESTRICT source,
                                       ByteCount size) noexcept {
	TRIVIAL_ASSERT(isAligned(destination, bytes(TRIVIAL_MEMORY_OPS_BLOCK_GRANULARITY)));
	TRIVIAL_ASSERT(isAligned(source, bytes(TRIVIAL_MEMORY_OPS_BLOCK_GRANULARITY)));

	return copy(std::assume_aligned<TRIVIAL_MEMORY_OPS_BLOCK_GRANULARITY>(static_cast<std::byte*>(destination)),
	            std::assume_aligned<TRIVIAL_MEMORY_OPS_BLOCK_GRANULARITY>(static_cast<const std::byte*>(source)),
	            size);
}

TRIVIAL_FORCE_INLINE void* fillAligned(void* destination, std::uint8_t pattern, ByteCount size) noexcept {
	TRIVIAL_ASSERT(isAligned(destination, bytes(TRIVIAL_MEMORY_OPS_BLOCK_GRANULARITY)));

	return fill(std::assume_aligned<TRIVIAL_MEMORY_OPS_BLOCK_GRANULARITY>(static_cast<std::byte*>(destination)),
	            pattern,
	            size);
}

TRIVIAL_FORCE_INLINE void* zeroAligned(void* destination, ByteCount size) noexcept {
	return fillAligned(destination, 0, size);
}

TRIVIAL_FORCE_INLINE void* zeroBlock(void* block, ByteCount blockSize) noexcept {
	constexpr std::size_t kGranularity = TRIVIAL_MEMORY_OPS_BLOCK_GRANULARITY;
	constexpr std::size_t kStoreBytes = TRIVIAL_MEMORY_OPS_ZERO_BLOCK_STORE_BYTES;
	const std::size_t kSize = blockSize.toSize();

	TRIVIAL_ASSERT(kSize % kGranularity == 0);
	TRIVIAL_ASSERT(kSize > 0);
	TRIVIAL_ASSERT(isAligned(block, bytes(kGranularity)));
	if (kSize >= TRIVIAL_MEMORY_OPS_BLOCK_ALIGNMENT) {
		TRIVIAL_ASSERT(isAligned(block, bytes(TRIVIAL_MEMORY_OPS_BLOCK_ALIGNMENT)));
	}

	if (kSize < kStoreBytes) [[unlikely]] {
		zero(block, bytes(kGranularity));
		return block;
	}

	TRIVIAL_ASSERT(isAligned(block, bytes(TRIVIAL_MEMORY_OPS_BLOCK_ALIGNMENT)));
	std::byte* const kStart = std::assume_aligned<TRIVIAL_MEMORY_OPS_BLOCK_ALIGNMENT>(static_cast<std::byte*>(block));
	std::byte* const kEnd = pointerPlus(kStart, blockSize);

	if (kSize < TRIVIAL_MEMORY_OPS_ZERO_BLOCK_FOUR_STORE_LIMIT) [[likely]] {
		const std::size_t kOffset = (kSize >> 1U) & kStoreBytes;
		TRIVIAL_ASSERT(kSize < 2 * kStoreBytes ? kOffset == 0 : kOffset == kStoreBytes);

		zero(kStart, bytes(kStoreBytes));
		zero(pointerPlus(kStart, bytes(kOffset)), bytes(kStoreBytes));
		zeroBackward(pointerMinus(kEnd, bytes(kOffset)), bytes(kStoreBytes));
		zeroBackward(kEnd, bytes(kStoreBytes));
		return block;
	}

	if (kSize <= TRIVIAL_MEMORY_OPS_ZERO_BLOCK_TWO_HALF_LIMIT) [[likely]] {
		zero(kStart, bytes(TRIVIAL_MEMORY_OPS_ZERO_BLOCK_TWO_HALF_LIMIT / 2));
		zeroBackward(kEnd, bytes(TRIVIAL_MEMORY_OPS_ZERO_BLOCK_TWO_HALF_LIMIT / 2));
		return block;
	}

	return zeroAligned(std::assume_aligned<kGranularity>(static_cast<std::byte*>(block)), blockSize);
}

[[nodiscard]] inline bool isZero(const void* source, ByteCount size) noexcept {
	return std::ranges::all_of(std::span(static_cast<const std::byte*>(source), size.toSize()), [](std::byte value) {
		return value == std::byte{0};
	});
}

} // namespace trivial::memory

#undef TRIVIAL_MEMORY_OPS_BLOCK_GRANULARITY
#undef TRIVIAL_MEMORY_OPS_BLOCK_ALIGNMENT
#undef TRIVIAL_MEMORY_OPS_ZERO_BLOCK_STORE_BYTES
#undef TRIVIAL_MEMORY_OPS_ZERO_BLOCK_FOUR_STORE_LIMIT
#undef TRIVIAL_MEMORY_OPS_ZERO_BLOCK_TWO_HALF_LIMIT

#endif // TRIVIAL_CORE_MEMORY_MEMORY_OPS_H

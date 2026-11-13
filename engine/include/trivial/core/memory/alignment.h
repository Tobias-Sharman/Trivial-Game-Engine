#ifndef TRIVIAL_CORE_MEMORY_ALIGNMENT_H
#define TRIVIAL_CORE_MEMORY_ALIGNMENT_H

#include <bit>
#include <cstdint>

#include <trivial/core/bits/bits.h>
#include <trivial/core/compiler.h>
#include <trivial/core/memory/byte_count.h>
#include <trivial/core/memory/pointer_arithmetic.h>

namespace trivial::memory {

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr bool isAligned(ByteCount value, ByteCount alignment) noexcept {
	return bits::isAligned(value.count(), alignment.count());
}

[[nodiscard]] TRIVIAL_FORCE_INLINE bool isAligned(const void* pointer, ByteCount alignment) noexcept {
	return bits::isAligned(std::bit_cast<std::uint64_t>(pointer), alignment.count());
}

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr ByteCount alignUp(ByteCount value, ByteCount alignment) noexcept {
	return bytes(bits::alignUp(value.count(), alignment.count()));
}

template <typename T>
[[nodiscard]] TRIVIAL_FORCE_INLINE T* alignUp(T* pointer, ByteCount alignment) noexcept {
	const std::uint64_t kAddress = std::bit_cast<std::uint64_t>(pointer);

	return pointerPlus(pointer, bytes(bits::alignUp(kAddress, alignment.count()) - kAddress));
}

template <typename T>
[[nodiscard]] TRIVIAL_FORCE_INLINE T* alignDown(T* pointer, ByteCount alignment) noexcept {
	const std::uint64_t kAddress = std::bit_cast<std::uint64_t>(pointer);

	return pointerMinus(pointer, bytes(kAddress - bits::alignDown(kAddress, alignment.count())));
}

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr ByteCount alignDown(ByteCount value, ByteCount alignment) noexcept {
	return bytes(bits::alignDown(value.count(), alignment.count()));
}

template <typename T>
TRIVIAL_FORCE_INLINE void advanceToAlignment(T*& pointer, ByteCount alignment) noexcept {
	pointer = alignUp(pointer, alignment);
}

template <typename T>
TRIVIAL_FORCE_INLINE void retreatToAlignment(T*& pointer, ByteCount alignment) noexcept {
	pointer = alignDown(pointer, alignment);
}

} // namespace trivial::memory

#endif // TRIVIAL_CORE_MEMORY_ALIGNMENT_H

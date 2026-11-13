#ifndef TRIVIAL_CORE_MEMORY_POINTER_ARITHMETIC_H
#define TRIVIAL_CORE_MEMORY_POINTER_ARITHMETIC_H

#include <bit>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <trivial/core/assert.h>
#include <trivial/core/compiler.h>
#include <trivial/core/memory/byte_count.h>

namespace trivial::memory {

template <typename From, typename To>
concept PointeeCastableTo = !std::is_same_v<To, From> && (std::is_const_v<To> || !std::is_const_v<From>);

template <typename T>
[[nodiscard]] TRIVIAL_FORCE_INLINE T* pointerPlus(T* pointer, ByteCount offset) noexcept {
	using Byte = std::conditional_t<std::is_const_v<T>, const std::byte, std::byte>;
	using Void = std::conditional_t<std::is_const_v<T>, const void, void>;

	Void* const kRaw = pointer;
	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	Void* const kSum = static_cast<Byte*>(kRaw) + offset.toSize();
	return static_cast<T*>(kSum);
}

template <typename To, PointeeCastableTo<To> From>
[[nodiscard]] TRIVIAL_FORCE_INLINE To* pointerPlus(From* pointer, ByteCount offset) noexcept {
	using Void = std::conditional_t<std::is_const_v<From>, const void, void>;

	Void* const kRaw = pointer;
	return static_cast<To*>(pointerPlus(kRaw, offset));
}

template <typename T>
[[nodiscard]] TRIVIAL_FORCE_INLINE T* pointerMinus(T* pointer, ByteCount offset) noexcept {
	using Byte = std::conditional_t<std::is_const_v<T>, const std::byte, std::byte>;
	using Void = std::conditional_t<std::is_const_v<T>, const void, void>;

	Void* const kRaw = pointer;
	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	Void* const kDifference = static_cast<Byte*>(kRaw) - offset.toSize();
	return static_cast<T*>(kDifference);
}

template <typename To, PointeeCastableTo<To> From>
[[nodiscard]] TRIVIAL_FORCE_INLINE To* pointerMinus(From* pointer, ByteCount offset) noexcept {
	using Void = std::conditional_t<std::is_const_v<From>, const void, void>;

	Void* const kRaw = pointer;
	return static_cast<To*>(pointerMinus(kRaw, offset));
}

[[nodiscard]] TRIVIAL_FORCE_INLINE ByteCount pointerDifference(const void* to, const void* from) noexcept {
	const std::uint64_t kTo = std::bit_cast<std::uint64_t>(to);
	const std::uint64_t kFrom = std::bit_cast<std::uint64_t>(from);
	TRIVIAL_ASSUME(kTo >= kFrom);

	return bytes(kTo - kFrom);
}

template <typename T>
TRIVIAL_FORCE_INLINE void advancePointer(T*& pointer, ByteCount offset) noexcept {
	pointer = pointerPlus(pointer, offset);
}

template <typename T>
TRIVIAL_FORCE_INLINE void retreatPointer(T*& pointer, ByteCount offset) noexcept {
	pointer = pointerMinus(pointer, offset);
}

} // namespace trivial::memory

#endif // TRIVIAL_CORE_MEMORY_POINTER_ARITHMETIC_H

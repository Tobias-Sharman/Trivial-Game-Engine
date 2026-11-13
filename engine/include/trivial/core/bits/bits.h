#ifndef TRIVIAL_CORE_BITS_BITS_H
#define TRIVIAL_CORE_BITS_BITS_H

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <trivial/core/assert.h>
#include <trivial/core/bits/bits_constants.h>
#include <trivial/core/compiler.h>
#include <trivial/core/math/concepts.h>
#include <trivial/core/math/scalar.h>

#if TRIVIAL_COMPILER_MSVC
#include <intrin.h>

#include <trivial/core/platform.h>

#if TRIVIAL_ARCH_X86_64
#include <trivial/core/cpu/cpu_features_x86_64.h>
#endif // TRIVIAL_ARCH_X86_64
#endif // TRIVIAL_COMPILER_MSVC

#define TRIVIAL_BITS_SWAR_PAIRS 0b01010101'01010101'01010101'01010101'01010101'01010101'01010101'01010101ULL
#define TRIVIAL_BITS_SWAR_NIBBLES 0b00110011'00110011'00110011'00110011'00110011'00110011'00110011'00110011ULL
#define TRIVIAL_BITS_SWAR_BYTES 0b00001111'00001111'00001111'00001111'00001111'00001111'00001111'00001111ULL
#define TRIVIAL_BITS_SWAR_BYTE_LANES 0b00000001'00000001'00000001'00000001'00000001'00000001'00000001'00000001ULL

namespace trivial::bits {

template <typename T>
concept BitValue = math::UnsignedIntegral<T> && !std::same_as<T, bool> && !std::same_as<T, char8_t>
                   && !std::same_as<T, char16_t> && !std::same_as<T, char32_t> && !std::same_as<T, wchar_t>;

template <BitValue T>
using RegisterFor = std::conditional_t<sizeof(T) == sizeof(std::uint64_t), std::uint64_t, std::uint32_t>;

[[nodiscard]] constexpr std::uint64_t trailingZerosPortable64(std::uint64_t value) noexcept {
	for (std::uint64_t i = 0; i < TRIVIAL_BITS_BITS_IN(std::uint64_t); ++i) {
		if (((value >> i) & 1U) != 0) {
			return i;
		}
	}

	return TRIVIAL_BITS_BITS_IN(std::uint64_t);
}

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr std::uint64_t trailingZeros64(std::uint64_t value) noexcept {
	if consteval {
		return trailingZerosPortable64(value);
	}

#if TRIVIAL_COMPILER_MSVC
#if TRIVIAL_ARCH_X86_64
	if constexpr (cpu::isTargetFeature(cpu::X86_64Feature::Bmi1)) {
		return _tzcnt_u64(value);
	}
#endif // TRIVIAL_ARCH_X86_64

	if (value == 0) {
		return TRIVIAL_BITS_BITS_IN(std::uint64_t);
	}

	unsigned long index = 0;
	_BitScanForward64(&index, value);
	return index;
#else
	return value == 0 ? TRIVIAL_BITS_BITS_IN(std::uint64_t) : static_cast<std::uint64_t>(__builtin_ctzll(value));
#endif // TRIVIAL_COMPILER_MSVC
}

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr std::uint32_t trailingZeros32(std::uint32_t value) noexcept {
	if consteval {
		return value == 0 ? TRIVIAL_BITS_BITS_IN(std::uint32_t)
		                  : static_cast<std::uint32_t>(trailingZerosPortable64(value));
	}

#if TRIVIAL_COMPILER_MSVC
#if TRIVIAL_ARCH_X86_64
	if constexpr (cpu::isTargetFeature(cpu::X86_64Feature::Bmi1)) {
		return _tzcnt_u32(value);
	}
#endif // TRIVIAL_ARCH_X86_64

	if (value == 0) {
		return TRIVIAL_BITS_BITS_IN(std::uint32_t);
	}

	unsigned long index = 0;
	_BitScanForward(&index, value);
	return index;
#else
	return value == 0 ? TRIVIAL_BITS_BITS_IN(std::uint32_t) : static_cast<std::uint32_t>(__builtin_ctz(value));
#endif // TRIVIAL_COMPILER_MSVC
}

template <BitValue Result, BitValue T>
[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr Result trailingZeros(T value) noexcept {
	if constexpr (sizeof(T) == sizeof(std::uint64_t)) {
		return static_cast<Result>(trailingZeros64(value));
	} else if constexpr (sizeof(T) == sizeof(std::uint32_t)) {
		return static_cast<Result>(trailingZeros32(value));
	} else {
		constexpr std::uint32_t kGuardBit = std::uint32_t{1} << TRIVIAL_BITS_BITS_IN(T);
		return static_cast<Result>(trailingZeros32(value | kGuardBit));
	}
}

template <BitValue Result, BitValue T>
[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr Result trailingZerosNonZero(T value) noexcept {
	TRIVIAL_ASSUME(value != 0);
	return trailingZeros<Result>(value);
}

template <BitValue Result, BitValue T>
[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr Result trailingOnes(T value) noexcept {
	return trailingZeros<Result>(static_cast<T>(~value));
}

[[nodiscard]] constexpr std::uint64_t leadingZerosPortable64(std::uint64_t value) noexcept {
	const std::uint64_t kTopBit = std::uint64_t{1} << (TRIVIAL_BITS_BITS_IN(std::uint64_t) - 1);

	for (std::uint64_t i = 0; i < TRIVIAL_BITS_BITS_IN(std::uint64_t); ++i) {
		if ((value & (kTopBit >> i)) != 0) {
			return i;
		}
	}

	return TRIVIAL_BITS_BITS_IN(std::uint64_t);
}

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr std::uint64_t leadingZeros64(std::uint64_t value) noexcept {
	if consteval {
		return leadingZerosPortable64(value);
	}

#if TRIVIAL_COMPILER_MSVC
#if TRIVIAL_ARCH_X86_64
	if constexpr (cpu::isTargetFeature(cpu::X86_64Feature::Lzcnt)) {
		return _lzcnt_u64(value);
	}

	if (value == 0) {
		return TRIVIAL_BITS_BITS_IN(std::uint64_t);
	}

	unsigned long index = 0;
	_BitScanReverse64(&index, value);
	return static_cast<std::uint64_t>(index ^ (TRIVIAL_BITS_BITS_IN(std::uint64_t) - 1));
#else
	return _CountLeadingZeros64(value);
#endif // TRIVIAL_ARCH_X86_64
#else
	return value == 0 ? TRIVIAL_BITS_BITS_IN(std::uint64_t) : static_cast<std::uint64_t>(__builtin_clzll(value));
#endif // TRIVIAL_COMPILER_MSVC
}

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr std::uint32_t leadingZeros32(std::uint32_t value) noexcept {
	if consteval {
		return static_cast<std::uint32_t>(leadingZerosPortable64(value) - TRIVIAL_BITS_BITS_IN(std::uint32_t));
	}

#if TRIVIAL_COMPILER_MSVC
#if TRIVIAL_ARCH_X86_64
	if constexpr (cpu::isTargetFeature(cpu::X86_64Feature::Lzcnt)) {
		return _lzcnt_u32(value);
	}

	if (value == 0) {
		return TRIVIAL_BITS_BITS_IN(std::uint32_t);
	}

	unsigned long index = 0;
	_BitScanReverse(&index, value);
	return static_cast<std::uint32_t>(index ^ (TRIVIAL_BITS_BITS_IN(std::uint32_t) - 1));
#else
	return _CountLeadingZeros(value);
#endif // TRIVIAL_ARCH_X86_64
#else
	return value == 0 ? TRIVIAL_BITS_BITS_IN(std::uint32_t) : static_cast<std::uint32_t>(__builtin_clz(value));
#endif // TRIVIAL_COMPILER_MSVC
}

template <BitValue Result, BitValue T>
[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr Result leadingZeros(T value) noexcept {
	if constexpr (sizeof(T) == sizeof(std::uint64_t)) {
		return static_cast<Result>(leadingZeros64(value));
	} else {
		constexpr std::uint32_t kPadding = TRIVIAL_BITS_BITS_IN(std::uint32_t) - TRIVIAL_BITS_BITS_IN(T);
		return static_cast<Result>(leadingZeros32(value) - kPadding);
	}
}

template <BitValue Result, BitValue T>
[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr Result leadingZerosNonZero(T value) noexcept {
	TRIVIAL_ASSUME(value != 0);
	return leadingZeros<Result>(value);
}

template <BitValue Result, BitValue T>
[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr Result leadingOnes(T value) noexcept {
	return leadingZeros<Result>(static_cast<T>(~value));
}

template <BitValue Result, BitValue T>
[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr Result significantBits(T value) noexcept {
	return static_cast<Result>(TRIVIAL_BITS_BITS_IN(T) - leadingZeros<std::size_t>(value));
}

[[nodiscard]] constexpr std::uint64_t countOnesPortable64(std::uint64_t value) noexcept {
	value -= (value >> 1U) & TRIVIAL_BITS_SWAR_PAIRS;
	value = (value & TRIVIAL_BITS_SWAR_NIBBLES) + ((value >> 2U) & TRIVIAL_BITS_SWAR_NIBBLES);
	value = (value + (value >> 4U)) & TRIVIAL_BITS_SWAR_BYTES;

	return (value * TRIVIAL_BITS_SWAR_BYTE_LANES) >> (TRIVIAL_BITS_BITS_IN(std::uint64_t) - TRIVIAL_BITS_BITS_PER_BYTE);
}

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr std::uint64_t countOnes64(std::uint64_t value) noexcept {
	if consteval {
		return countOnesPortable64(value);
	}

#if TRIVIAL_COMPILER_MSVC
#if TRIVIAL_ARCH_X86_64
	if constexpr (cpu::isTargetFeature(cpu::X86_64Feature::Popcnt)) {
		return __popcnt64(value);
	}

	return countOnesPortable64(value);
#else
	return _CountOneBits64(value);
#endif // TRIVIAL_ARCH_X86_64
#else
	return static_cast<std::uint64_t>(__builtin_popcountll(value));
#endif // TRIVIAL_COMPILER_MSVC
}

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr std::uint32_t countOnes32(std::uint32_t value) noexcept {
	if consteval {
		return static_cast<std::uint32_t>(countOnesPortable64(value));
	}

#if TRIVIAL_COMPILER_MSVC
#if TRIVIAL_ARCH_X86_64
	if constexpr (cpu::isTargetFeature(cpu::X86_64Feature::Popcnt)) {
		return __popcnt(value);
	}

	return static_cast<std::uint32_t>(countOnesPortable64(value));
#else
	return _CountOneBits(value);
#endif // TRIVIAL_ARCH_X86_64
#else
	return static_cast<std::uint32_t>(__builtin_popcount(value));
#endif // TRIVIAL_COMPILER_MSVC
}

template <BitValue Result, BitValue T>
[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr Result countOnes(T value) noexcept {
	if constexpr (sizeof(T) == sizeof(std::uint64_t)) {
		return static_cast<Result>(countOnes64(value));
	} else {
		return static_cast<Result>(countOnes32(value));
	}
}

template <BitValue T>
[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr bool isPowerOfTwo(T value) noexcept {
	return countOnes<std::size_t>(value) == 1;
}

template <BitValue T>
[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr bool isAligned(T value, std::type_identity_t<T> alignment) noexcept {
	const RegisterFor<T> kValue = value;
	const RegisterFor<T> kAlignment = alignment;
	TRIVIAL_ASSUME(kAlignment != 0);
	TRIVIAL_ASSUME((kAlignment & (kAlignment - 1)) == 0);

	return (kValue & (kAlignment - 1)) == 0;
}

template <BitValue T>
[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr T alignUp(T value, std::type_identity_t<T> alignment) noexcept {
	const RegisterFor<T> kValue = value;
	const RegisterFor<T> kAlignment = alignment;
	TRIVIAL_ASSUME(kAlignment != 0);
	TRIVIAL_ASSUME((kAlignment & (kAlignment - 1)) == 0);

	return static_cast<T>((kValue + kAlignment - 1) & ~(kAlignment - 1));
}

template <BitValue T>
[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr T alignDown(T value, std::type_identity_t<T> alignment) noexcept {
	const RegisterFor<T> kValue = value;
	const RegisterFor<T> kAlignment = alignment;
	TRIVIAL_ASSUME(kAlignment != 0);
	TRIVIAL_ASSUME((kAlignment & (kAlignment - 1)) == 0);

	return static_cast<T>(kValue & ~(kAlignment - 1));
}

template <BitValue T>
[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr T lowestSetBit(T value) noexcept {
	const RegisterFor<T> kValue = value;
	return static_cast<T>(kValue & (~kValue + 1));
}

template <BitValue T>
[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr T clearLowestSetBit(T value) noexcept {
	const RegisterFor<T> kValue = value;
	return static_cast<T>(kValue & (kValue - 1));
}

template <BitValue T>
[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr T bitRangeMask(std::size_t start, std::size_t count) noexcept {
	TRIVIAL_ASSUME(count > 0);
	TRIVIAL_ASSERT(start + count <= TRIVIAL_BITS_BITS_IN(T));

	const RegisterFor<T> kAllSet = static_cast<T>(~T{0});
	return static_cast<T>((kAllSet >> (TRIVIAL_BITS_BITS_IN(T) - count)) << start);
}

template <BitValue T>
[[nodiscard]] constexpr std::size_t findRunOfOnes(T value, std::size_t count) noexcept {
	TRIVIAL_ASSERT(count > 0);
	TRIVIAL_ASSERT(count <= TRIVIAL_BITS_BITS_IN(T));

	RegisterFor<T> runStarts = value;
	std::size_t runLength = 1;

	while (runLength < count && runStarts != 0) {
		const std::size_t kGrowth = math::min(runLength, count - runLength);
		runStarts &= runStarts >> kGrowth;
		runLength += kGrowth;
	}

	return runStarts == 0 ? TRIVIAL_BITS_BITS_IN(T) : trailingZerosNonZero<std::size_t>(runStarts);
}

} // namespace trivial::bits

#undef TRIVIAL_BITS_SWAR_PAIRS
#undef TRIVIAL_BITS_SWAR_NIBBLES
#undef TRIVIAL_BITS_SWAR_BYTES
#undef TRIVIAL_BITS_SWAR_BYTE_LANES

#endif // TRIVIAL_CORE_BITS_BITS_H

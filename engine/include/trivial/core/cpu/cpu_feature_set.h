#ifndef TRIVIAL_CORE_CPU_CPU_FEATURE_SET_H
#define TRIVIAL_CORE_CPU_CPU_FEATURE_SET_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

#include <trivial/core/assert.h>
#include <trivial/core/compiler.h>

#define TRIVIAL_CPU_FEATURE_SET_WORD_BITS static_cast<std::size_t>(std::numeric_limits<std::uint64_t>::digits)

namespace trivial::cpu {

template <typename Feature>
struct FeatureSet {
	std::array<std::uint64_t,
	           (static_cast<std::size_t>(Feature::Count) + TRIVIAL_CPU_FEATURE_SET_WORD_BITS - 1)
	               / TRIVIAL_CPU_FEATURE_SET_WORD_BITS>
	    words{};

	TRIVIAL_FORCE_INLINE constexpr void insert(Feature feature) noexcept {
		TRIVIAL_ASSUME(feature < Feature::Count);
		const std::size_t kIndex = static_cast<std::size_t>(feature);
		const std::size_t kWord = kIndex / TRIVIAL_CPU_FEATURE_SET_WORD_BITS;
		const std::uint64_t kMask = std::uint64_t{1} << (kIndex % TRIVIAL_CPU_FEATURE_SET_WORD_BITS);

		words[kWord] |= kMask;
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr bool contains(Feature feature) const noexcept {
		TRIVIAL_ASSUME(feature < Feature::Count);
		const std::size_t kIndex = static_cast<std::size_t>(feature);
		const std::size_t kWord = kIndex / TRIVIAL_CPU_FEATURE_SET_WORD_BITS;
		const std::uint64_t kMask = std::uint64_t{1} << (kIndex % TRIVIAL_CPU_FEATURE_SET_WORD_BITS);

		return (words[kWord] & kMask) != 0;
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr bool containsAll(const FeatureSet& other) const noexcept {
		for (std::size_t word = 0; word < words.size(); ++word) {
			if ((other.words[word] & ~words[word]) != 0) {
				return false;
			}
		}

		return true;
	}
};

} // namespace trivial::cpu

#undef TRIVIAL_CPU_FEATURE_SET_WORD_BITS

#endif // TRIVIAL_CORE_CPU_CPU_FEATURE_SET_H

#ifndef TRIVIAL_CORE_CPU_CPU_FEATURE_SET_H
#define TRIVIAL_CORE_CPU_CPU_FEATURE_SET_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

#include <trivial/core/assert.h>
#include <trivial/core/compiler.h>

#define TRIVIAL_CPU_FEATURE_SET_MASK_BITS static_cast<std::size_t>(std::numeric_limits<std::uint64_t>::digits)

namespace trivial::cpu {

template <typename Feature>
struct FeatureSet {
	std::array<std::uint64_t,
	           (static_cast<std::size_t>(Feature::Count) + TRIVIAL_CPU_FEATURE_SET_MASK_BITS - 1)
	               / TRIVIAL_CPU_FEATURE_SET_MASK_BITS>
	    masks{};

	TRIVIAL_FORCE_INLINE constexpr void insert(Feature feature) noexcept {
		TRIVIAL_ASSUME(feature < Feature::Count);
		const std::size_t kFeatureIndex = static_cast<std::size_t>(feature);
		const std::size_t kMaskIndex = kFeatureIndex / TRIVIAL_CPU_FEATURE_SET_MASK_BITS;
		const std::uint64_t kBit = std::uint64_t{1} << (kFeatureIndex % TRIVIAL_CPU_FEATURE_SET_MASK_BITS);

		masks[kMaskIndex] |= kBit;
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr bool contains(Feature feature) const noexcept {
		TRIVIAL_ASSUME(feature < Feature::Count);
		const std::size_t kFeatureIndex = static_cast<std::size_t>(feature);
		const std::size_t kMaskIndex = kFeatureIndex / TRIVIAL_CPU_FEATURE_SET_MASK_BITS;
		const std::uint64_t kBit = std::uint64_t{1} << (kFeatureIndex % TRIVIAL_CPU_FEATURE_SET_MASK_BITS);

		return (masks[kMaskIndex] & kBit) != 0;
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr bool containsAll(const FeatureSet& other) const noexcept {
		for (std::size_t i = 0; i < masks.size(); ++i) {
			if ((other.masks[i] & ~masks[i]) != 0) {
				return false;
			}
		}

		return true;
	}
};

} // namespace trivial::cpu

#undef TRIVIAL_CPU_FEATURE_SET_MASK_BITS

#endif // TRIVIAL_CORE_CPU_CPU_FEATURE_SET_H

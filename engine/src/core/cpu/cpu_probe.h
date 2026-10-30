#ifndef TRIVIAL_SRC_CORE_CPU_CPU_PROBE_H
#define TRIVIAL_SRC_CORE_CPU_CPU_PROBE_H

#include <cstddef>
#include <cstdint>
#include <string_view>

#include <trivial/core/assert.h>
#include <trivial/core/cpu/cpu.h>
#include <trivial/core/cpu/cpu_feature_set.h>
#include <trivial/core/log.h>
#include <trivial/core/platform.h>

#if TRIVIAL_ARCH_X86_64
#include <trivial/core/cpu/cpu_features_x86_64.h>

#elif TRIVIAL_ARCH_ARM64
#include <trivial/core/cpu/cpu_features_arm64.h>

#endif // Cpu architecture check

#if TRIVIAL_PLATFORM_HAS_CPU_AFFINITY
#include <array>
#include <memory>
#include <span>

#define TRIVIAL_CPU_RAW_ID_NONE std::uint32_t{0xFFFFFFFF}
#endif // TRIVIAL_PLATFORM_HAS_CPU_AFFINITY

#define TRIVIAL_CPU_PROBE_HAS_MAIN_IDS (TRIVIAL_ARCH_ARM64 && (TRIVIAL_PLATFORM_LINUX || TRIVIAL_PLATFORM_WINDOWS))

namespace trivial::cpu {

[[nodiscard]] constexpr std::uint32_t extractBits(std::uint32_t value,
                                                  std::uint32_t highBit,
                                                  std::uint32_t lowBit) noexcept {
	TRIVIAL_ASSUME(lowBit <= highBit);
	TRIVIAL_ASSUME(highBit < 32);

	const std::uint32_t kMask = ~std::uint32_t{0} >> (31 - (highBit - lowBit));

	return (value >> lowBit) & kMask;
}

static_assert(
    [] {
	    const std::uint32_t kAllBits = 0xFFFFFFFF;
	    const std::uint32_t kHighestBit = 31;

	    return extractBits(kAllBits, kHighestBit, 0) == kAllBits;
    }(),
    "extractBits should read a field of the full width");

#if TRIVIAL_PLATFORM_HAS_CPU_AFFINITY
enum class TopologyType : std::uint8_t {
	Core,
	Package,
	NumaNode,

	L1DataCache,
	L1InstructionCache,
	L2Cache,
	L3Cache,

	Count,
};

static_assert(static_cast<std::size_t>(TopologyType::Count) - static_cast<std::size_t>(TopologyType::L1DataCache)
                  == static_cast<std::size_t>(CacheKind::Count),
              "TopologyType cache entries should follow CacheKind order");

using TopologyIds = std::array<std::uint32_t, static_cast<std::size_t>(TopologyType::Count)>;

struct RawLogicalProcessor {
	std::uint32_t processorNumber;
	std::uint32_t efficiencyClass;
	TopologyIds topologyIds;
#if TRIVIAL_ARCH_ARM64
	Arm64MainId mainId;
#endif // TRIVIAL_ARCH_ARM64
};

struct RawCache {
	std::uint32_t id;
	std::uint32_t sizeBytes;
	std::uint16_t associativity;
	CacheKind kind;
};

struct RawTopology {
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
	std::unique_ptr<RawLogicalProcessor[]> logicalProcessors;
	std::unique_ptr<RawCache[]> caches; // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
	std::uint32_t logicalProcessorCount;
	std::uint32_t cacheCount;
	std::uint16_t cacheLineSize;
};
#endif // TRIVIAL_PLATFORM_HAS_CPU_AFFINITY

void copyBrand(std::string_view reported) noexcept;

#if TRIVIAL_ARCH_X86_64
using ArchitectureFeature = X86_64Feature;
#elif TRIVIAL_ARCH_ARM64
using ArchitectureFeature = Arm64Feature;
#endif // Cpu architecture check

using PrerequisitesFunction = FeatureSet<ArchitectureFeature>(ArchitectureFeature) noexcept;

template <PrerequisitesFunction PrerequisitesOf>
void insertFeaturesWithPrerequisites(const FeatureSet<ArchitectureFeature>& reported) noexcept {
	for (std::size_t i = 0; i < static_cast<std::size_t>(ArchitectureFeature::Count); ++i) {
		const ArchitectureFeature kFeature = static_cast<ArchitectureFeature>(i);

		if (!reported.contains(kFeature)) {
			continue;
		}

		const FeatureSet<ArchitectureFeature> kPrerequisites = PrerequisitesOf(kFeature);

		if (reported.containsAll(kPrerequisites)) {
			detail::g_detectedCapabilities.identity.features.insert(kFeature);
			continue;
		}

		TRIVIAL_LOG_WARNING_PREFIX("Cpu",
		                           "a feature was reported without all of its prerequisites, so it is not used:");
		TRIVIAL_LOG_WARNING_PREFIX("Cpu", featureName(kFeature));
		TRIVIAL_LOG_WARNING_PREFIX("Cpu", "its missing prerequisites:");

		for (std::size_t j = 0; j < static_cast<std::size_t>(ArchitectureFeature::Count); ++j) {
			const ArchitectureFeature kPrerequisite = static_cast<ArchitectureFeature>(j);

			if (kPrerequisites.contains(kPrerequisite) && !reported.contains(kPrerequisite)) {
				TRIVIAL_LOG_WARNING_PREFIX("Cpu", featureName(kPrerequisite));
			}
		}
	}
}

void probeIdentity() noexcept;
#if TRIVIAL_PLATFORM_HAS_CPU_AFFINITY
[[nodiscard]] RawTopology probeRawTopology() noexcept;
#if TRIVIAL_CPU_PROBE_HAS_MAIN_IDS
void probeMainIds(std::span<RawLogicalProcessor> logicalProcessors) noexcept;
#endif // TRIVIAL_CPU_PROBE_HAS_MAIN_IDS
#else
void probeTopology() noexcept;
#endif // TRIVIAL_PLATFORM_HAS_CPU_AFFINITY
void probeRunningTranslated() noexcept;

} // namespace trivial::cpu

#if TRIVIAL_PLATFORM_HAS_CPU_AFFINITY && !defined(TRIVIAL_CPU_PROBE_IMPLEMENTATION)
#undef TRIVIAL_CPU_RAW_ID_NONE
#endif // TRIVIAL_PLATFORM_HAS_CPU_AFFINITY && !defined(TRIVIAL_CPU_PROBE_IMPLEMENTATION)

#endif // TRIVIAL_SRC_CORE_CPU_CPU_PROBE_H

#ifndef TRIVIAL_SRC_CORE_CPU_CPU_PROBE_H
#define TRIVIAL_SRC_CORE_CPU_CPU_PROBE_H

#include <trivial/core/platform.h>

#if TRIVIAL_PLATFORM_HAS_CPU_AFFINITY
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

#include <trivial/core/cpu/cpu.h>

#if TRIVIAL_ARCH_ARM64
#include <trivial/core/cpu/cpu_features_arm64.h>
#endif // TRIVIAL_ARCH_ARM64

#define TRIVIAL_CPU_RAW_ID_NONE std::uint32_t{0xFFFFFFFF}
#endif // TRIVIAL_PLATFORM_HAS_CPU_AFFINITY

namespace trivial::cpu {

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

void probeIdentity() noexcept;
#if TRIVIAL_PLATFORM_HAS_CPU_AFFINITY
[[nodiscard]] RawTopology probeRawTopology() noexcept;
#else
void probeTopology() noexcept;
#endif // TRIVIAL_PLATFORM_HAS_CPU_AFFINITY
void probeRunningTranslated() noexcept;

} // namespace trivial::cpu

#if TRIVIAL_PLATFORM_HAS_CPU_AFFINITY && !defined(TRIVIAL_CPU_PROBE_IMPLEMENTATION)
#undef TRIVIAL_CPU_RAW_ID_NONE
#endif // TRIVIAL_PLATFORM_HAS_CPU_AFFINITY && !defined(TRIVIAL_CPU_PROBE_IMPLEMENTATION)

#endif // TRIVIAL_SRC_CORE_CPU_CPU_PROBE_H

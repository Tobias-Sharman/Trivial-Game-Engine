#ifndef TRIVIAL_CORE_CPU_CPU_H
#define TRIVIAL_CORE_CPU_CPU_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include <trivial/core/assert.h>
#include <trivial/core/compiler.h>
#include <trivial/core/config.h>
#include <trivial/core/cpu/cpu_feature_set.h>
#include <trivial/core/platform.h>

#if TRIVIAL_ARCH_X86_64
#include <trivial/core/cpu/cpu_features_x86_64.h>

#elif TRIVIAL_ARCH_ARM64
#include <trivial/core/cpu/cpu_features_arm64.h>

#endif // Cpu architecture check

#define TRIVIAL_CPU_INVALID_CACHE_INDEX std::uint16_t{0xFFFF}
#define TRIVIAL_CPU_CACHE_ASSOCIATIVITY_UNKNOWN std::uint16_t{0}
#define TRIVIAL_CPU_CACHE_FULLY_ASSOCIATIVE std::uint16_t{0xFFFF}
#define TRIVIAL_CPU_VENDOR_CAPACITY std::size_t{16}
#define TRIVIAL_CPU_BRAND_CAPACITY std::size_t{64}

namespace trivial::cpu {

using VendorString = std::array<char, TRIVIAL_CPU_VENDOR_CAPACITY>;
using BrandString = std::array<char, TRIVIAL_CPU_BRAND_CAPACITY>;

enum class CacheKind : std::uint8_t {
	L1Data,
	L1Instruction,
	L2,
	L3,

	Count,
};

struct CacheInfo {
	std::uint32_t sizeBytes;
	std::uint16_t associativity;
	std::uint16_t sharingLogicalProcessorCount;
	CacheKind kind;
};

// Mainly was due to a packing annoyance can be dropped if it really need be
// changed
static_assert(sizeof(CacheInfo) == 12, "CacheInfo should stay 12 bytes"); // NOLINT(readability-magic-numbers)

#if TRIVIAL_PLATFORM_HAS_CPU_AFFINITY
using CacheIndices = std::array<std::uint16_t, static_cast<std::size_t>(CacheKind::Count)>;

struct LogicalProcessorInfo {
	std::uint32_t processorNumber;
	std::uint32_t core;
	std::uint16_t package;
	std::uint16_t numaNode;
	CacheIndices cacheIndices;
	std::uint8_t efficiencyClass;
#if TRIVIAL_ARCH_ARM64
	Arm64MainId mainId;
#endif // TRIVIAL_ARCH_ARM64
};

#else
using CachesByKind = std::array<CacheInfo, static_cast<std::size_t>(CacheKind::Count)>;

struct EfficiencyClassInfo {
	std::uint32_t logicalProcessorCount;
	std::uint32_t coreCount;
	CachesByKind caches;
};
#endif // TRIVIAL_PLATFORM_HAS_CPU_AFFINITY

namespace detail {

struct Identity {
#if TRIVIAL_ARCH_X86_64
	FeatureSet<X86_64Feature> features;
	X86_64Info architectureInfo;
#elif TRIVIAL_ARCH_ARM64
	FeatureSet<Arm64Feature> features;
#endif // Cpu architecture check

	VendorString vendor{};
	BrandString brand{};
};

struct Topology {
#if TRIVIAL_PLATFORM_HAS_CPU_AFFINITY
	LogicalProcessorInfo* logicalProcessors = nullptr;
	CacheInfo* caches = nullptr;
	std::uint32_t logicalProcessorCount = 0;
	std::uint32_t cacheCount = 0;
#else
	EfficiencyClassInfo* efficiencyClasses = nullptr;
	std::uint32_t logicalProcessorCount = 0;
#endif // TRIVIAL_PLATFORM_HAS_CPU_AFFINITY
	std::uint32_t physicalCoreCount = 0;
	std::uint32_t performanceCoreCount = 0;
	std::uint32_t efficiencyCoreCount = 0;
	std::uint32_t efficiencyClassCount = 0;
	std::uint32_t packageCount = 0;
	std::uint32_t numaNodeCount = 0;
	std::uint16_t cacheLineSize = 0;
};

struct DetectedCapabilities {
	Identity identity;
	Topology topology;
	bool runningTranslated = false;
#if TRIVIAL_ENABLE_ASSERTS
	bool initialised = false;
#endif // TRIVIAL_ENABLE_ASSERTS
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
inline constinit DetectedCapabilities g_detectedCapabilities;

} // namespace detail

#if TRIVIAL_ARCH_X86_64
[[nodiscard]] TRIVIAL_FORCE_INLINE bool isDetected(X86_64Feature feature) noexcept {
	TRIVIAL_ASSERT(detail::g_detectedCapabilities.initialised);
	return detail::g_detectedCapabilities.identity.features.contains(feature);
}

namespace detail {

template <X86_64Feature Feature>
[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr bool has() noexcept {
	if constexpr (isTargetFeature(Feature)) {
		return true;
	} else {
		return isDetected(Feature);
	}
}

} // namespace detail

[[nodiscard]] TRIVIAL_FORCE_INLINE const X86_64Info& architectureInfo() noexcept {
	TRIVIAL_ASSERT(detail::g_detectedCapabilities.initialised);
	return detail::g_detectedCapabilities.identity.architectureInfo;
}

#elif TRIVIAL_ARCH_ARM64
[[nodiscard]] TRIVIAL_FORCE_INLINE bool isDetected(Arm64Feature feature) noexcept {
	TRIVIAL_ASSERT(detail::g_detectedCapabilities.initialised);
	return detail::g_detectedCapabilities.identity.features.contains(feature);
}

namespace detail {

template <Arm64Feature Feature>
[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr bool has() noexcept {
	if constexpr (isTargetFeature(Feature)) {
		return true;
	} else {
		return isDetected(Feature);
	}
}

} // namespace detail

#endif // Cpu architecture check

#define TRIVIAL_CPU_HAS(feature) (::trivial::cpu::detail::has<(feature)>())

[[nodiscard]] TRIVIAL_FORCE_INLINE const char* vendor() noexcept {
	TRIVIAL_ASSERT(detail::g_detectedCapabilities.initialised);
	return detail::g_detectedCapabilities.identity.vendor.data();
}

[[nodiscard]] TRIVIAL_FORCE_INLINE const char* brand() noexcept {
	TRIVIAL_ASSERT(detail::g_detectedCapabilities.initialised);
	return detail::g_detectedCapabilities.identity.brand.data();
}

[[nodiscard]] TRIVIAL_FORCE_INLINE std::uint32_t logicalProcessorCount() noexcept {
	TRIVIAL_ASSERT(detail::g_detectedCapabilities.initialised);
	return detail::g_detectedCapabilities.topology.logicalProcessorCount;
}

#if TRIVIAL_PLATFORM_HAS_CPU_AFFINITY
[[nodiscard]] TRIVIAL_FORCE_INLINE std::span<const LogicalProcessorInfo> logicalProcessors() noexcept {
	TRIVIAL_ASSERT(detail::g_detectedCapabilities.initialised);
	return {detail::g_detectedCapabilities.topology.logicalProcessors,
	        detail::g_detectedCapabilities.topology.logicalProcessorCount};
}

[[nodiscard]] TRIVIAL_FORCE_INLINE std::span<const CacheInfo> caches() noexcept {
	TRIVIAL_ASSERT(detail::g_detectedCapabilities.initialised);
	return {detail::g_detectedCapabilities.topology.caches, detail::g_detectedCapabilities.topology.cacheCount};
}

#else
[[nodiscard]] TRIVIAL_FORCE_INLINE std::span<const EfficiencyClassInfo> efficiencyClasses() noexcept {
	TRIVIAL_ASSERT(detail::g_detectedCapabilities.initialised);
	return {detail::g_detectedCapabilities.topology.efficiencyClasses,
	        detail::g_detectedCapabilities.topology.efficiencyClassCount};
}

#endif // TRIVIAL_PLATFORM_HAS_CPU_AFFINITY

[[nodiscard]] TRIVIAL_FORCE_INLINE std::uint32_t physicalCoreCount() noexcept {
	TRIVIAL_ASSERT(detail::g_detectedCapabilities.initialised);
	return detail::g_detectedCapabilities.topology.physicalCoreCount;
}

[[nodiscard]] TRIVIAL_FORCE_INLINE std::uint32_t performanceCoreCount() noexcept {
	TRIVIAL_ASSERT(detail::g_detectedCapabilities.initialised);
	return detail::g_detectedCapabilities.topology.performanceCoreCount;
}

[[nodiscard]] TRIVIAL_FORCE_INLINE std::uint32_t efficiencyCoreCount() noexcept {
	TRIVIAL_ASSERT(detail::g_detectedCapabilities.initialised);
	return detail::g_detectedCapabilities.topology.efficiencyCoreCount;
}

[[nodiscard]] TRIVIAL_FORCE_INLINE std::uint32_t efficiencyClassCount() noexcept {
	TRIVIAL_ASSERT(detail::g_detectedCapabilities.initialised);
	return detail::g_detectedCapabilities.topology.efficiencyClassCount;
}

[[nodiscard]] TRIVIAL_FORCE_INLINE std::uint32_t packageCount() noexcept {
	TRIVIAL_ASSERT(detail::g_detectedCapabilities.initialised);
	return detail::g_detectedCapabilities.topology.packageCount;
}

[[nodiscard]] TRIVIAL_FORCE_INLINE std::uint32_t numaNodeCount() noexcept {
	TRIVIAL_ASSERT(detail::g_detectedCapabilities.initialised);
	return detail::g_detectedCapabilities.topology.numaNodeCount;
}

[[nodiscard]] TRIVIAL_FORCE_INLINE std::uint16_t cacheLineSize() noexcept {
	TRIVIAL_ASSERT(detail::g_detectedCapabilities.initialised);
	return detail::g_detectedCapabilities.topology.cacheLineSize;
}

[[nodiscard]] TRIVIAL_FORCE_INLINE bool runningTranslated() noexcept {
	TRIVIAL_ASSERT(detail::g_detectedCapabilities.initialised);
	return detail::g_detectedCapabilities.runningTranslated;
}

[[nodiscard]] std::uint32_t availableLogicalProcessorCount() noexcept;
[[nodiscard]] std::uint32_t quotaLogicalProcessorCount() noexcept;

} // namespace trivial::cpu

#undef TRIVIAL_CPU_VENDOR_CAPACITY
#undef TRIVIAL_CPU_BRAND_CAPACITY

#endif // TRIVIAL_CORE_CPU_CPU_H

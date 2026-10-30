#define TRIVIAL_CPU_PROBE_IMPLEMENTATION
#include "core/cpu/cpu_capabilities.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string_view>

#include <trivial/core/config.h>
#include <trivial/core/cpu/cpu.h>
#include <trivial/core/cpu/cpu_config.h>
#include <trivial/core/cpu/cpu_feature_set.h>
#include <trivial/core/log.h>
#include <trivial/core/platform.h>

#if TRIVIAL_ARCH_X86_64
#include <trivial/core/cpu/cpu_features_x86_64.h>

#elif TRIVIAL_ARCH_ARM64
#include <trivial/core/cpu/cpu_features_arm64.h>

#endif // Cpu architecture check

#include "core/cpu/cpu_probe.h"

#if TRIVIAL_PLATFORM_HAS_CPU_AFFINITY
#include <array>
#include <limits>
#include <memory>
#include <span>
#endif // TRIVIAL_PLATFORM_HAS_CPU_AFFINITY

namespace {

#if TRIVIAL_PLATFORM_HAS_CPU_AFFINITY
[[nodiscard]] std::size_t findRawCache(std::span<const trivial::cpu::RawCache> caches,
                                       std::uint32_t id,
                                       trivial::cpu::CacheKind kind) noexcept {
	for (std::size_t i = 0; i < caches.size(); ++i) {
		if (caches[i].id == id && caches[i].kind == kind) {
			return i;
		}
	}

	return caches.size();
}

void sortAndScaleTopologyIds(std::span<trivial::cpu::RawLogicalProcessor> rawLogicalProcessors,
                             trivial::cpu::TopologyType type,
                             std::uint32_t& count) noexcept {
	const std::size_t kType = static_cast<std::size_t>(type);

	std::ranges::sort(rawLogicalProcessors,
	                  [kType](const trivial::cpu::RawLogicalProcessor& first,
	                          const trivial::cpu::RawLogicalProcessor& second) noexcept {
		                  return first.topologyIds[kType] < second.topologyIds[kType];
	                  });

	std::uint32_t previous = rawLogicalProcessors.front().topologyIds[kType];
	std::uint32_t scaled = 0;

	for (trivial::cpu::RawLogicalProcessor& logicalProcessor : rawLogicalProcessors) {
		if (logicalProcessor.topologyIds[kType] != previous) {
			previous = logicalProcessor.topologyIds[kType];
			++scaled;
		}

		logicalProcessor.topologyIds[kType] = scaled;
	}

	count = scaled + 1;
}

[[nodiscard]] bool sortAndScaleCacheIds(std::span<trivial::cpu::RawLogicalProcessor> rawLogicalProcessors,
                                        std::span<const trivial::cpu::RawCache> rawCaches,
                                        trivial::cpu::CacheKind kind,
                                        std::span<trivial::cpu::CacheInfo> caches,
                                        std::uint32_t& cacheCount) noexcept {
	const std::size_t kType
	    = static_cast<std::size_t>(trivial::cpu::TopologyType::L1DataCache) + static_cast<std::size_t>(kind);

	std::ranges::sort(rawLogicalProcessors,
	                  [kType](const trivial::cpu::RawLogicalProcessor& first,
	                          const trivial::cpu::RawLogicalProcessor& second) noexcept {
		                  return first.topologyIds[kType] < second.topologyIds[kType];
	                  });

	std::uint32_t previous = TRIVIAL_CPU_RAW_ID_NONE;

	for (trivial::cpu::RawLogicalProcessor& logicalProcessor : rawLogicalProcessors) {
		const std::uint32_t kId = logicalProcessor.topologyIds[kType];

		if (kId == TRIVIAL_CPU_RAW_ID_NONE) {
			break;
		}

		if (kId != previous) {
			const std::size_t kRawIndex = findRawCache(rawCaches, kId, kind);

			if (kRawIndex == rawCaches.size()) {
				return false;
			}

			const trivial::cpu::RawCache& kRawCache = rawCaches[kRawIndex];
			caches[cacheCount] = trivial::cpu::CacheInfo{
			    .sizeBytes = kRawCache.sizeBytes,
			    .associativity = kRawCache.associativity,
			    .sharingLogicalProcessorCount = 0,
			    .kind = kind,
			};

			previous = kId;
			++cacheCount;
		}

		++caches[cacheCount - 1].sharingLogicalProcessorCount;
		logicalProcessor.topologyIds[kType] = cacheCount - 1;
	}

	return true;
}

void discardCaches(std::span<trivial::cpu::RawLogicalProcessor> rawLogicalProcessors) noexcept {
	for (trivial::cpu::RawLogicalProcessor& logicalProcessor : rawLogicalProcessors) {
		for (std::size_t i = 0; i < static_cast<std::size_t>(trivial::cpu::CacheKind::Count); ++i) {
			logicalProcessor.topologyIds[static_cast<std::size_t>(trivial::cpu::TopologyType::L1DataCache) + i]
			    = TRIVIAL_CPU_RAW_ID_NONE;
		}
	}
}

void sortLogicalProcessorsAndScaleEfficiencyClasses(std::span<trivial::cpu::RawLogicalProcessor> rawLogicalProcessors,
                                                    std::uint32_t& classCount) noexcept {
	std::ranges::sort(
	    rawLogicalProcessors,
	    [](const trivial::cpu::RawLogicalProcessor& first, const trivial::cpu::RawLogicalProcessor& second) noexcept {
		    constexpr std::size_t kCore = static_cast<std::size_t>(trivial::cpu::TopologyType::Core);

		    if (first.efficiencyClass != second.efficiencyClass) {
			    return first.efficiencyClass < second.efficiencyClass;
		    }

		    if (first.topologyIds[kCore] != second.topologyIds[kCore]) {
			    return first.topologyIds[kCore] < second.topologyIds[kCore];
		    }

		    return first.processorNumber < second.processorNumber;
	    });

	std::uint32_t previous = rawLogicalProcessors.front().efficiencyClass;
	std::uint32_t rank = 0;

	for (trivial::cpu::RawLogicalProcessor& logicalProcessor : rawLogicalProcessors) {
		if (logicalProcessor.efficiencyClass != previous) {
			if (rank == std::numeric_limits<std::uint8_t>::max()) [[unlikely]] {
				TRIVIAL_LOG_ERROR_PREFIX("Cpu",
				                         "the OS reported more efficiency classes than supported, so they are unknown");

				for (trivial::cpu::RawLogicalProcessor& unranked : rawLogicalProcessors) {
					unranked.efficiencyClass = 0;
				}

				classCount = 0;
				return;
			}

			previous = logicalProcessor.efficiencyClass;
			++rank;
		}

		logicalProcessor.efficiencyClass = rank;
	}

	classCount = rank + 1;
}

void countEfficiencyAndPerformanceCores(std::span<const trivial::cpu::RawLogicalProcessor> rawLogicalProcessors,
                                        trivial::cpu::detail::Topology& topology) noexcept {
	if (topology.efficiencyClassCount == 0) {
		topology.efficiencyCoreCount = 0;
		topology.performanceCoreCount = 0;
		return;
	}

	constexpr std::size_t kCore = static_cast<std::size_t>(trivial::cpu::TopologyType::Core);
	const std::uint32_t kPerformanceRank = topology.efficiencyClassCount - 1;
	std::uint32_t efficiencyCoreCount = 0;

	for (std::size_t i = 0;
	     i < rawLogicalProcessors.size() && rawLogicalProcessors[i].efficiencyClass != kPerformanceRank;
	     ++i) {
		if (i == 0 || rawLogicalProcessors[i - 1].topologyIds[kCore] != rawLogicalProcessors[i].topologyIds[kCore]) {
			++efficiencyCoreCount;
		}
	}

	topology.efficiencyCoreCount = efficiencyCoreCount;
	topology.performanceCoreCount = topology.physicalCoreCount - efficiencyCoreCount;
}

void fillLogicalProcessors(std::span<const trivial::cpu::RawLogicalProcessor> rawLogicalProcessors,
                           std::span<trivial::cpu::LogicalProcessorInfo> logicalProcessors) noexcept {
	for (std::size_t i = 0; i < rawLogicalProcessors.size(); ++i) {
		const trivial::cpu::RawLogicalProcessor& kRaw = rawLogicalProcessors[i];
		trivial::cpu::CacheIndices cacheIndices{};

		for (std::size_t j = 0; j < cacheIndices.size(); ++j) {
			const std::uint32_t kId
			    = kRaw.topologyIds[static_cast<std::size_t>(trivial::cpu::TopologyType::L1DataCache) + j];

			if (kId == TRIVIAL_CPU_RAW_ID_NONE) {
				cacheIndices[j] = TRIVIAL_CPU_INVALID_CACHE_INDEX;
			} else {
				cacheIndices[j] = static_cast<std::uint16_t>(kId);
			}
		}

		logicalProcessors[i] = trivial::cpu::LogicalProcessorInfo{
		    .processorNumber = kRaw.processorNumber,
		    .core = kRaw.topologyIds[static_cast<std::size_t>(trivial::cpu::TopologyType::Core)],
		    .package = static_cast<std::uint16_t>(
		        kRaw.topologyIds[static_cast<std::size_t>(trivial::cpu::TopologyType::Package)]),
		    .numaNode = static_cast<std::uint16_t>(
		        kRaw.topologyIds[static_cast<std::size_t>(trivial::cpu::TopologyType::NumaNode)]),
		    .cacheIndices = cacheIndices,
		    .efficiencyClass = static_cast<std::uint8_t>(kRaw.efficiencyClass),
#if TRIVIAL_ARCH_ARM64
		    .mainId = kRaw.mainId,
#endif // TRIVIAL_ARCH_ARM64
		};
	}
}

void buildTopology() noexcept {
	const trivial::cpu::RawTopology kRaw = trivial::cpu::probeRawTopology();
	const std::span<trivial::cpu::RawLogicalProcessor> kRawLogicalProcessors{kRaw.logicalProcessors.get(),
	                                                                         kRaw.logicalProcessorCount};
	const std::span<const trivial::cpu::RawCache> kRawCaches{kRaw.caches.get(), kRaw.cacheCount};

	if (kRawLogicalProcessors.empty()) [[unlikely]] {
		return;
	}

	if (kRawLogicalProcessors.size() > std::numeric_limits<std::uint16_t>::max()) [[unlikely]] {
		TRIVIAL_LOG_ERROR_PREFIX("Cpu",
		                         "the OS reported more logical processors than supported, so the topology is unknown");
		return;
	}

#if TRIVIAL_CPU_PROBE_HAS_MAIN_IDS
	trivial::cpu::probeMainIds(kRawLogicalProcessors);
#endif // TRIVIAL_CPU_PROBE_HAS_MAIN_IDS

	trivial::cpu::detail::Topology& topology = trivial::cpu::detail::g_detectedCapabilities.topology;
	topology.logicalProcessorCount = kRaw.logicalProcessorCount;
	topology.cacheLineSize = kRaw.cacheLineSize;
	sortAndScaleTopologyIds(kRawLogicalProcessors, trivial::cpu::TopologyType::Core, topology.physicalCoreCount);
	sortAndScaleTopologyIds(kRawLogicalProcessors, trivial::cpu::TopologyType::Package, topology.packageCount);
	sortAndScaleTopologyIds(kRawLogicalProcessors, trivial::cpu::TopologyType::NumaNode, topology.numaNodeCount);

	std::span<const trivial::cpu::RawCache> rawCaches = kRawCaches;

	if (rawCaches.size() >= TRIVIAL_CPU_INVALID_CACHE_INDEX) [[unlikely]] {
		TRIVIAL_LOG_ERROR_PREFIX("Cpu", "the OS reported more caches than supported, so caches are unknown");
		discardCaches(kRawLogicalProcessors);
		rawCaches = {};
	}

	topology.caches = new trivial::cpu::CacheInfo[rawCaches.size()];
	const std::span<trivial::cpu::CacheInfo> kCaches{topology.caches, rawCaches.size()};
	std::uint32_t cacheCount = 0;

	const bool kCachesListed
	    = sortAndScaleCacheIds(kRawLogicalProcessors, rawCaches, trivial::cpu::CacheKind::L1Data, kCaches, cacheCount)
	      && sortAndScaleCacheIds(kRawLogicalProcessors,
	                              rawCaches,
	                              trivial::cpu::CacheKind::L1Instruction,
	                              kCaches,
	                              cacheCount)
	      && sortAndScaleCacheIds(kRawLogicalProcessors, rawCaches, trivial::cpu::CacheKind::L2, kCaches, cacheCount)
	      && sortAndScaleCacheIds(kRawLogicalProcessors, rawCaches, trivial::cpu::CacheKind::L3, kCaches, cacheCount);

	const bool kCachesValid = kCachesListed && cacheCount == rawCaches.size();

	if (!kCachesValid) [[unlikely]] {
		TRIVIAL_LOG_ERROR_PREFIX(
		    "Cpu",
		    "the OS reported caches that do not match its logical processors, so caches are unknown");
		discardCaches(kRawLogicalProcessors);
	}

	sortLogicalProcessorsAndScaleEfficiencyClasses(kRawLogicalProcessors, topology.efficiencyClassCount);
	countEfficiencyAndPerformanceCores(kRawLogicalProcessors, topology);

	if (kCachesValid) {
		topology.cacheCount = cacheCount;
	} else {
		delete[] topology.caches;
		topology.caches = nullptr;
	}

	topology.logicalProcessors = new trivial::cpu::LogicalProcessorInfo[kRawLogicalProcessors.size()];
	fillLogicalProcessors(kRawLogicalProcessors, {topology.logicalProcessors, kRawLogicalProcessors.size()});
}
#endif // TRIVIAL_PLATFORM_HAS_CPU_AFFINITY

} // namespace

namespace trivial::cpu {

void copyBrand(std::string_view reported) noexcept {
	const std::size_t kLength = std::min(reported.find('\0'), reported.size());
	BrandString& brand = detail::g_detectedCapabilities.identity.brand;
	const std::size_t kCopied = std::min(kLength, brand.size() - 1);

	std::memcpy(brand.data(), reported.data(), kCopied);
	brand[kCopied] = '\0';

	if (kLength > kCopied) {
		TRIVIAL_LOG_WARNING_PREFIX("Cpu", "the OS reported a brand longer than the brand string, so it is cut short");
	}
}

void initCapabilities() noexcept {
	probeIdentity();

	if constexpr (TRIVIAL_CPU_ENABLE_FLOOR_CHECK) {
		const FeatureSet<ArchitectureFeature>& kDetected = detail::g_detectedCapabilities.identity.features;

		if (!kDetected.containsAll(detail::targetFeatures())) [[unlikely]] {
			TRIVIAL_LOG_FATAL_PREFIX("Cpu", "missing features this build requires:");

			for (std::size_t i = 0; i < static_cast<std::size_t>(ArchitectureFeature::Count); ++i) {
				const ArchitectureFeature kFeature = static_cast<ArchitectureFeature>(i);

				if (isTargetFeature(kFeature) && !kDetected.contains(kFeature)) {
					TRIVIAL_LOG_FATAL_PREFIX("Cpu", featureName(kFeature));
				}
			}

			probeRunningTranslated();

			if (detail::g_detectedCapabilities.runningTranslated) {
				TRIVIAL_LOG_FATAL_PREFIX("Cpu", "running under x86-64 translation, which may not provide them");
			}

			std::abort();
		}
	}

#if TRIVIAL_PLATFORM_HAS_CPU_AFFINITY
	buildTopology();
#else
	probeTopology();
#endif // TRIVIAL_PLATFORM_HAS_CPU_AFFINITY

	if constexpr (TRIVIAL_CPU_ENABLE_CACHE_LINE_CHECK) {
		const std::uint16_t kCacheLineSize = detail::g_detectedCapabilities.topology.cacheLineSize;

		if (kCacheLineSize != 0 && kCacheLineSize != TRIVIAL_PLATFORM_CACHE_LINE_SIZE) {
			TRIVIAL_LOG_WARNING_PREFIX("Cpu", "L1 data cache line size differs from TRIVIAL_PLATFORM_CACHE_LINE_SIZE");
		}
	}

	probeRunningTranslated();

#if TRIVIAL_ENABLE_ASSERTS
	detail::g_detectedCapabilities.initialised = true;
#endif // TRIVIAL_ENABLE_ASSERTS
}

void freeCapabilities() noexcept {
#if TRIVIAL_PLATFORM_HAS_CPU_AFFINITY
	delete[] detail::g_detectedCapabilities.topology.logicalProcessors;
	delete[] detail::g_detectedCapabilities.topology.caches;
#else
	delete[] detail::g_detectedCapabilities.topology.efficiencyClasses;
#endif // TRIVIAL_PLATFORM_HAS_CPU_AFFINITY
	detail::g_detectedCapabilities.topology = {};

#if TRIVIAL_ENABLE_ASSERTS
	detail::g_detectedCapabilities.initialised = false;
#endif // TRIVIAL_ENABLE_ASSERTS
}

} // namespace trivial::cpu

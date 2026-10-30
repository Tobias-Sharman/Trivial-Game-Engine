#define TRIVIAL_CPU_PROBE_IMPLEMENTATION
#include "core/cpu/cpu_probe.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX

#include <windows.h> // IWYU pragma: keep

#include <trivial/core/cpu/cpu.h>
#include <trivial/core/log.h>
#include <trivial/core/platform.h>

#define TRIVIAL_CPU_WINDOWS_GROUP_PROCESSORS std::uint32_t{64}

namespace {

using ProcessorRecord = SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX;

template <typename Visit>
void visitRecords(std::span<const std::byte> buffer, Visit visit) noexcept {
	for (std::size_t offset = 0; offset < buffer.size();) {
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		const ProcessorRecord& kRecord = *reinterpret_cast<const ProcessorRecord*>(&buffer[offset]);

		if (kRecord.Size == 0 || kRecord.Size > buffer.size() - offset) {
			return;
		}

		visit(kRecord);
		offset += kRecord.Size;
	}
}

[[nodiscard]] std::span<const GROUP_AFFINITY> groupMasks(const ProcessorRecord& record,
                                                         const GROUP_AFFINITY& first,
                                                         WORD groupCount) noexcept {
	const std::uintptr_t kOffset = std::bit_cast<std::uintptr_t>(&first) - std::bit_cast<std::uintptr_t>(&record);
	const std::size_t kFitting = kOffset < record.Size ? (record.Size - kOffset) / sizeof(GROUP_AFFINITY) : 0;
	const std::size_t kReported = groupCount == 0 ? std::size_t{1} : std::size_t{groupCount};

	return {&first, std::min(kReported, kFitting)};
}

[[nodiscard]] trivial::cpu::CacheKind cacheKind(const CACHE_RELATIONSHIP& cache) noexcept {
	if (cache.Level == 1 && cache.Type == CacheData) {
		return trivial::cpu::CacheKind::L1Data;
	}

	if (cache.Level == 1 && cache.Type == CacheInstruction) {
		return trivial::cpu::CacheKind::L1Instruction;
	}

	if (cache.Level == 2 && cache.Type == CacheUnified) {
		return trivial::cpu::CacheKind::L2;
	}

	if (cache.Level == 3 && cache.Type == CacheUnified) {
		return trivial::cpu::CacheKind::L3;
	}

	return trivial::cpu::CacheKind::Count;
}

void assignTopologyId(std::span<trivial::cpu::RawLogicalProcessor> logicalProcessors,
                      std::span<const GROUP_AFFINITY> masks,
                      trivial::cpu::TopologyType type,
                      std::uint32_t id) noexcept {
	for (trivial::cpu::RawLogicalProcessor& logicalProcessor : logicalProcessors) {
		const std::uint32_t kGroup = logicalProcessor.processorNumber / TRIVIAL_CPU_WINDOWS_GROUP_PROCESSORS;
		const std::uint32_t kBit = logicalProcessor.processorNumber % TRIVIAL_CPU_WINDOWS_GROUP_PROCESSORS;
		const bool kCovered = std::ranges::any_of(masks, [kGroup, kBit](const GROUP_AFFINITY& mask) noexcept {
			return mask.Group == kGroup && ((mask.Mask >> kBit) & 1U) != 0;
		});

		if (kCovered) {
			logicalProcessor.topologyIds[static_cast<std::size_t>(type)] = id;
		}
	}
}

void fillLogicalProcessors(std::span<const std::byte> buffer,
                           std::span<trivial::cpu::RawLogicalProcessor> logicalProcessors) noexcept {
	std::size_t logicalProcessorIndex = 0;
	std::uint32_t coreId = 0;

	visitRecords(buffer, [&](const ProcessorRecord& record) noexcept {
		if (record.Relationship != RelationProcessorCore) {
			return;
		}

		const PROCESSOR_RELATIONSHIP& kCore = record.Processor; // NOLINT(cppcoreguidelines-pro-type-union-access)

		for (const GROUP_AFFINITY& kMask : groupMasks(record, kCore.GroupMask[0], kCore.GroupCount)) {
			for (KAFFINITY bits = kMask.Mask; bits != 0; bits &= bits - 1) {
				trivial::cpu::RawLogicalProcessor& logicalProcessor = logicalProcessors[logicalProcessorIndex];
				logicalProcessor.processorNumber = (std::uint32_t{kMask.Group} * TRIVIAL_CPU_WINDOWS_GROUP_PROCESSORS)
				                                   + static_cast<std::uint32_t>(std::countr_zero(bits));
				logicalProcessor.efficiencyClass = kCore.EfficiencyClass;
				logicalProcessor.topologyIds.fill(TRIVIAL_CPU_RAW_ID_NONE);
				logicalProcessor.topologyIds[static_cast<std::size_t>(trivial::cpu::TopologyType::Core)] = coreId;
				logicalProcessor.topologyIds[static_cast<std::size_t>(trivial::cpu::TopologyType::Package)] = 0;
				logicalProcessor.topologyIds[static_cast<std::size_t>(trivial::cpu::TopologyType::NumaNode)] = 0;
				++logicalProcessorIndex;
			}
		}

		++coreId;
	});
}

[[nodiscard]] std::uint16_t fillGroupedTopology(std::span<const std::byte> buffer,
                                                std::span<trivial::cpu::RawLogicalProcessor> logicalProcessors,
                                                std::span<trivial::cpu::RawCache> caches) noexcept {
	std::uint32_t packageId = 0;
	std::uint32_t cacheIndex = 0;
	std::uint16_t cacheLineSize = 0;

	visitRecords(buffer, [&](const ProcessorRecord& record) noexcept {
		if (record.Relationship == RelationProcessorPackage) {
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
			const PROCESSOR_RELATIONSHIP& kPackage = record.Processor;
			const std::span<const GROUP_AFFINITY> kMasks
			    = groupMasks(record, kPackage.GroupMask[0], kPackage.GroupCount);
			assignTopologyId(logicalProcessors, kMasks, trivial::cpu::TopologyType::Package, packageId);
			++packageId;
			return;
		}

		if (record.Relationship == RelationNumaNode) {
			const NUMA_NODE_RELATIONSHIP& kNode = record.NumaNode; // NOLINT(cppcoreguidelines-pro-type-union-access)
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
			const std::span<const GROUP_AFFINITY> kMasks = groupMasks(record, kNode.GroupMask, kNode.GroupCount);
			assignTopologyId(logicalProcessors, kMasks, trivial::cpu::TopologyType::NumaNode, kNode.NodeNumber);
			return;
		}

		if (record.Relationship != RelationCache) {
			return;
		}

		const CACHE_RELATIONSHIP& kCache = record.Cache; // NOLINT(cppcoreguidelines-pro-type-union-access)
		const trivial::cpu::CacheKind kKind = cacheKind(kCache);

		if (kKind == trivial::cpu::CacheKind::Count) {
			return;
		}

		const trivial::cpu::TopologyType kType = static_cast<trivial::cpu::TopologyType>(
		    static_cast<std::size_t>(trivial::cpu::TopologyType::L1DataCache) + static_cast<std::size_t>(kKind));

		caches[cacheIndex] = trivial::cpu::RawCache{
		    .id = cacheIndex,
		    .sizeBytes = kCache.CacheSize,
		    .associativity = kCache.Associativity == CACHE_FULLY_ASSOCIATIVE ? TRIVIAL_CPU_CACHE_FULLY_ASSOCIATIVE
			                                                                 : std::uint16_t{kCache.Associativity},
		    .kind = kKind,
		};

		if (kKind == trivial::cpu::CacheKind::L1Data && cacheLineSize == 0) {
			cacheLineSize = kCache.LineSize;
		}

		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
		const std::span<const GROUP_AFFINITY> kMasks = groupMasks(record, kCache.GroupMask, kCache.GroupCount);
		assignTopologyId(logicalProcessors, kMasks, kType, cacheIndex);
		++cacheIndex;
	});

	return cacheLineSize;
}

} // namespace

namespace trivial::cpu {

[[nodiscard]] RawTopology probeRawTopology() noexcept {
	DWORD length = 0;
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
	std::unique_ptr<std::byte[]> buffer;

	while (GetLogicalProcessorInformationEx(RelationAll,
	                                        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	                                        reinterpret_cast<ProcessorRecord*>(buffer.get()),
	                                        &length)
	       == FALSE) {
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) [[unlikely]] {
			TRIVIAL_LOG_ERROR_PREFIX("Cpu", "the OS did not report the processor topology, so it is unknown");

			return RawTopology{
			    .logicalProcessors = nullptr,
			    .caches = nullptr,
			    .logicalProcessorCount = 0,
			    .cacheCount = 0,
			    .cacheLineSize = 0,
			};
		}

		// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
		buffer = std::make_unique_for_overwrite<std::byte[]>(length);
	}

	const std::span<const std::byte> kBuffer{buffer.get(), length};
	std::uint32_t logicalProcessorCount = 0;
	std::uint32_t cacheCount = 0;
	bool unsupportedCache = false;

	visitRecords(kBuffer, [&](const ProcessorRecord& record) noexcept {
		if (record.Relationship == RelationProcessorCore) {
			const PROCESSOR_RELATIONSHIP& kCore = record.Processor; // NOLINT(cppcoreguidelines-pro-type-union-access)

			for (const GROUP_AFFINITY& kMask : groupMasks(record, kCore.GroupMask[0], kCore.GroupCount)) {
				logicalProcessorCount += static_cast<std::uint32_t>(std::popcount(kMask.Mask));
			}
		} else if (record.Relationship == RelationCache) {
			const CACHE_RELATIONSHIP& kCache = record.Cache; // NOLINT(cppcoreguidelines-pro-type-union-access)

			if (cacheKind(kCache) == CacheKind::Count) {
				unsupportedCache = true;
			} else {
				++cacheCount;
			}
		}
	});

	if (unsupportedCache) {
		TRIVIAL_LOG_WARNING_PREFIX("Cpu", "the OS reported caches other than L1, L2 and L3, so those are left out");
	}

	RawTopology topology{
	    .logicalProcessors{new RawLogicalProcessor[logicalProcessorCount]},
	    .caches{new RawCache[cacheCount]},
	    .logicalProcessorCount = logicalProcessorCount,
	    .cacheCount = cacheCount,
	    .cacheLineSize = 0,
	};
	const std::span<RawLogicalProcessor> kLogicalProcessors{topology.logicalProcessors.get(), logicalProcessorCount};

	fillLogicalProcessors(kBuffer, kLogicalProcessors);
	topology.cacheLineSize = fillGroupedTopology(kBuffer, kLogicalProcessors, {topology.caches.get(), cacheCount});

	return topology;
}

void probeRunningTranslated() noexcept {
	if constexpr (TRIVIAL_ARCH_X86_64) {
		USHORT processMachine = 0;
		USHORT nativeMachine = 0;

		detail::g_detectedCapabilities.runningTranslated
		    = IsWow64Process2(GetCurrentProcess(), &processMachine, &nativeMachine) != FALSE
		      && nativeMachine == IMAGE_FILE_MACHINE_ARM64;
	}
}

} // namespace trivial::cpu

#include "core/cpu/cpu_probe.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <span>
#include <sys/sysctl.h>

#include <trivial/core/cpu/cpu.h>
#include <trivial/core/log.h>

#define TRIVIAL_CPU_MACOS_SYSCTL_NAME_CAPACITY std::size_t{64}

namespace {

[[nodiscard]] std::array<char, TRIVIAL_CPU_MACOS_SYSCTL_NAME_CAPACITY> levelSysctlName(std::uint32_t level,
                                                                                       const char* field) noexcept {
	std::array<char, TRIVIAL_CPU_MACOS_SYSCTL_NAME_CAPACITY> name{};
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
	(void)std::snprintf(name.data(), name.size(), "hw.perflevel%u.%s", static_cast<unsigned int>(level), field);
	return name;
}

void probeCaches(std::span<trivial::cpu::EfficiencyClassInfo> classes) noexcept {
	for (std::size_t efficiencyClass = 0; efficiencyClass < classes.size(); ++efficiencyClass) {
		const std::uint32_t kLevel = static_cast<std::uint32_t>(classes.size() - 1 - efficiencyClass);
		trivial::cpu::EfficiencyClassInfo& classInfo = classes[efficiencyClass];
		const std::uint32_t kLogicalProcessorsPerCore = classInfo.logicalProcessorCount / classInfo.coreCount;
		std::size_t size = 0;

		std::uint32_t l1DataBytes = 0;
		std::uint32_t logicalProcessorsPerL1Data = 0;
		size = sizeof(l1DataBytes);

		if (sysctlbyname(levelSysctlName(kLevel, "l1dcachesize").data(), &l1DataBytes, &size, nullptr, 0) != 0
		    && errno != EINVAL) [[unlikely]] {
			TRIVIAL_LOG_ERROR_PREFIX("Cpu", "the OS did not report the L1 data cache size, so that cache is unknown");
			l1DataBytes = 0;
		}

		if (l1DataBytes != 0) {
			logicalProcessorsPerL1Data = kLogicalProcessorsPerCore;
		}

		classInfo.caches[static_cast<std::size_t>(trivial::cpu::CacheKind::L1Data)] = trivial::cpu::CacheInfo{
		    .sizeBytes = l1DataBytes,
		    .associativity = TRIVIAL_CPU_CACHE_ASSOCIATIVITY_UNKNOWN,
		    .sharingLogicalProcessorCount
		    = static_cast<std::uint16_t>(std::min(logicalProcessorsPerL1Data, classInfo.logicalProcessorCount)),
		    .kind = trivial::cpu::CacheKind::L1Data,
		};

		std::uint32_t l1InstructionBytes = 0;
		std::uint32_t logicalProcessorsPerL1Instruction = 0;
		size = sizeof(l1InstructionBytes);

		if (sysctlbyname(levelSysctlName(kLevel, "l1icachesize").data(), &l1InstructionBytes, &size, nullptr, 0) != 0
		    && errno != EINVAL) [[unlikely]] {
			TRIVIAL_LOG_ERROR_PREFIX("Cpu",
			                         "the OS did not report the L1 instruction cache size, so that cache is unknown");
			l1InstructionBytes = 0;
		}

		if (l1InstructionBytes != 0) {
			logicalProcessorsPerL1Instruction = kLogicalProcessorsPerCore;
		}

		classInfo.caches[static_cast<std::size_t>(trivial::cpu::CacheKind::L1Instruction)] = trivial::cpu::CacheInfo{
		    .sizeBytes = l1InstructionBytes,
		    .associativity = TRIVIAL_CPU_CACHE_ASSOCIATIVITY_UNKNOWN,
		    .sharingLogicalProcessorCount
		    = static_cast<std::uint16_t>(std::min(logicalProcessorsPerL1Instruction, classInfo.logicalProcessorCount)),
		    .kind = trivial::cpu::CacheKind::L1Instruction,
		};

		std::uint32_t l2Bytes = 0;
		std::uint32_t logicalProcessorsPerL2 = 0;
		size = sizeof(l2Bytes);

		if (sysctlbyname(levelSysctlName(kLevel, "l2cachesize").data(), &l2Bytes, &size, nullptr, 0) != 0
		    && errno != EINVAL) [[unlikely]] {
			TRIVIAL_LOG_ERROR_PREFIX("Cpu", "the OS did not report the L2 cache size, so that cache is unknown");
			l2Bytes = 0;
		}

		size = sizeof(logicalProcessorsPerL2);

		if (l2Bytes != 0
		    && (sysctlbyname(levelSysctlName(kLevel, "cpusperl2").data(), &logicalProcessorsPerL2, &size, nullptr, 0)
		            != 0
		        || logicalProcessorsPerL2 == 0)) [[unlikely]] {
			TRIVIAL_LOG_ERROR_PREFIX(
			    "Cpu",
			    "the OS did not report the logical processors sharing the L2 cache, so that cache is unknown");
			l2Bytes = 0;
			logicalProcessorsPerL2 = 0;
		}

		classInfo.caches[static_cast<std::size_t>(trivial::cpu::CacheKind::L2)] = trivial::cpu::CacheInfo{
		    .sizeBytes = l2Bytes,
		    .associativity = TRIVIAL_CPU_CACHE_ASSOCIATIVITY_UNKNOWN,
		    .sharingLogicalProcessorCount
		    = static_cast<std::uint16_t>(std::min(logicalProcessorsPerL2, classInfo.logicalProcessorCount)),
		    .kind = trivial::cpu::CacheKind::L2,
		};

		std::uint32_t l3Bytes = 0;
		std::uint32_t logicalProcessorsPerL3 = 0;
		size = sizeof(l3Bytes);

		if (sysctlbyname(levelSysctlName(kLevel, "l3cachesize").data(), &l3Bytes, &size, nullptr, 0) != 0
		    && errno != EINVAL) [[unlikely]] {
			TRIVIAL_LOG_ERROR_PREFIX("Cpu", "the OS did not report the L3 cache size, so that cache is unknown");
			l3Bytes = 0;
		}

		size = sizeof(logicalProcessorsPerL3);

		if (l3Bytes != 0
		    && (sysctlbyname(levelSysctlName(kLevel, "cpusperl3").data(), &logicalProcessorsPerL3, &size, nullptr, 0)
		            != 0
		        || logicalProcessorsPerL3 == 0)) [[unlikely]] {
			TRIVIAL_LOG_ERROR_PREFIX(
			    "Cpu",
			    "the OS did not report the logical processors sharing the L3 cache, so that cache is unknown");
			l3Bytes = 0;
			logicalProcessorsPerL3 = 0;
		}

		classInfo.caches[static_cast<std::size_t>(trivial::cpu::CacheKind::L3)] = trivial::cpu::CacheInfo{
		    .sizeBytes = l3Bytes,
		    .associativity = TRIVIAL_CPU_CACHE_ASSOCIATIVITY_UNKNOWN,
		    .sharingLogicalProcessorCount
		    = static_cast<std::uint16_t>(std::min(logicalProcessorsPerL3, classInfo.logicalProcessorCount)),
		    .kind = trivial::cpu::CacheKind::L3,
		};
	}
}

} // namespace

namespace trivial::cpu {

void probeTopology() noexcept {
	std::uint64_t levelCount = 0;
	std::size_t size = sizeof(levelCount);

	if (sysctlbyname("hw.nperflevels", &levelCount, &size, nullptr, 0) != 0 || levelCount == 0) [[unlikely]] {
		TRIVIAL_LOG_ERROR_PREFIX("Cpu", "the OS did not report valid performance levels, so the topology is unknown");
		return;
	}

	detail::Topology& topology = detail::g_detectedCapabilities.topology;
	topology.efficiencyClasses = new EfficiencyClassInfo[levelCount];
	const std::span<EfficiencyClassInfo> kClasses{topology.efficiencyClasses, static_cast<std::size_t>(levelCount)};

	for (std::size_t efficiencyClass = 0; efficiencyClass < kClasses.size(); ++efficiencyClass) {
		const std::uint32_t kLevel = static_cast<std::uint32_t>(kClasses.size() - 1 - efficiencyClass);
		const std::array<char, TRIVIAL_CPU_MACOS_SYSCTL_NAME_CAPACITY> kCoreCountName
		    = levelSysctlName(kLevel, "physicalcpu_max");
		const std::array<char, TRIVIAL_CPU_MACOS_SYSCTL_NAME_CAPACITY> kLogicalProcessorCountName
		    = levelSysctlName(kLevel, "logicalcpu_max");
		EfficiencyClassInfo& classInfo = kClasses[efficiencyClass];
		classInfo.coreCount = 0;
		classInfo.logicalProcessorCount = 0;
		bool countsValid = true;

		size = sizeof(classInfo.coreCount);
		countsValid &= sysctlbyname(kCoreCountName.data(), &classInfo.coreCount, &size, nullptr, 0) == 0;
		size = sizeof(classInfo.logicalProcessorCount);
		countsValid
		    &= sysctlbyname(kLogicalProcessorCountName.data(), &classInfo.logicalProcessorCount, &size, nullptr, 0)
		       == 0;
		countsValid = countsValid && classInfo.coreCount != 0 && classInfo.logicalProcessorCount >= classInfo.coreCount
		              && classInfo.logicalProcessorCount % classInfo.coreCount == 0;

		if (!countsValid) [[unlikely]] {
			TRIVIAL_LOG_ERROR_PREFIX("Cpu",
			                         "the OS did not report valid logical processor and core counts, so the topology "
			                         "is unknown");
			delete[] topology.efficiencyClasses;
			topology = {};
			return;
		}

		topology.logicalProcessorCount += classInfo.logicalProcessorCount;
		topology.physicalCoreCount += classInfo.coreCount;
	}

	std::uint64_t cacheLineSize = 0;
	size = sizeof(cacheLineSize);

	if (sysctlbyname("hw.cachelinesize", &cacheLineSize, &size, nullptr, 0) == 0) {
		topology.cacheLineSize = static_cast<std::uint16_t>(cacheLineSize);
	} else {
		TRIVIAL_LOG_ERROR_PREFIX("Cpu", "the OS did not report a cache line size, so it is unknown");
	}

	probeCaches(kClasses);

	topology.efficiencyClassCount = static_cast<std::uint32_t>(kClasses.size());
	topology.performanceCoreCount = kClasses.back().coreCount;
	topology.efficiencyCoreCount = topology.physicalCoreCount - topology.performanceCoreCount;
	topology.packageCount = 1;
	topology.numaNodeCount = 1;
}

void probeRunningTranslated() noexcept {
	int translated = 0;
	std::size_t size = sizeof(translated);

	detail::g_detectedCapabilities.runningTranslated
	    = sysctlbyname("sysctl.proc_translated", &translated, &size, nullptr, 0) == 0 && translated == 1;
}

} // namespace trivial::cpu

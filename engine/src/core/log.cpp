#include <trivial/core/log.h>

#include <trivial/core/config.h>

#if TRIVIAL_ENABLE_LOGGING

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <format>
#include <span>
#include <string>
#include <string_view>

#include <trivial/core/assert.h>
#include <trivial/core/compiler.h>
#include <trivial/core/cpu/cpu.h>
#include <trivial/core/memory/memory.h>
#include <trivial/core/memory/memory_config.h>
#include <trivial/core/memory/memory_constants.h>
#include <trivial/core/platform.h>
#include <trivial/core/time/time.h>
#include <trivial/core/time/time_constants.h>

#if TRIVIAL_ARCH_X86_64
#include <trivial/core/cpu/cpu_features_x86_64.h>

#elif TRIVIAL_ARCH_ARM64
#include <trivial/core/cpu/cpu_features_arm64.h>

#endif // Cpu architecture check

#define TRIVIAL_LOG_SYSTEM_INFO_LINE_WIDTH std::size_t{72}
#define TRIVIAL_LOG_SYSTEM_INFO_SIZE_UNIT_LIMIT std::uint64_t{1'000}
#define TRIVIAL_LOG_SYSTEM_INFO_SIZE_HUNDREDTHS std::uint64_t{100}

namespace {

template <typename... Args>
TRIVIAL_COLD void logInfo(const char* prefix, std::format_string<const Args&...> format, const Args&... args) noexcept {
	const std::string kText = std::format(format, args...);

	TRIVIAL_LOG_INFO_PREFIX(prefix, kText.c_str());
}

[[nodiscard]] TRIVIAL_COLD std::string formatSize(std::uint64_t bytes) noexcept {
	if (bytes == 0) {
		return "unknown";
	}

	const char* unit = "B";
	std::uint64_t divisor = 1;

	if (bytes >= TRIVIAL_LOG_SYSTEM_INFO_SIZE_UNIT_LIMIT) {
		unit = "KiB";
		divisor = TRIVIAL_MEMORY_BYTES_PER_KIB;
	}

	if (bytes >= TRIVIAL_LOG_SYSTEM_INFO_SIZE_UNIT_LIMIT * TRIVIAL_MEMORY_BYTES_PER_KIB) {
		unit = "MiB";
		divisor = TRIVIAL_MEMORY_BYTES_PER_MIB;
	}

	if (bytes >= TRIVIAL_LOG_SYSTEM_INFO_SIZE_UNIT_LIMIT * TRIVIAL_MEMORY_BYTES_PER_MIB) {
		unit = "GiB";
		divisor = TRIVIAL_MEMORY_BYTES_PER_GIB;
	}

	return std::format("{}.{:02} {}",
	                   bytes / divisor,
	                   bytes % divisor * TRIVIAL_LOG_SYSTEM_INFO_SIZE_HUNDREDTHS / divisor,
	                   unit);
}

#if TRIVIAL_PLATFORM_HAS_CPU_AFFINITY
TRIVIAL_COLD void appendCache(std::string& line, std::string_view name, std::uint16_t index) noexcept {
	if (index != TRIVIAL_CPU_INVALID_CACHE_INDEX) {
		const trivial::cpu::CacheInfo& kCache = trivial::cpu::caches()[index];

		line += std::format(", {} {} (#{}, shared by {})",
		                    name,
		                    formatSize(kCache.sizeBytes),
		                    index,
		                    kCache.sharingLogicalProcessorCount);
	}
}

#else
TRIVIAL_COLD void appendCache(std::string& line, std::string_view name, const trivial::cpu::CacheInfo& cache) noexcept {
	if (cache.sizeBytes != 0) {
		line += std::format(", {} {} (shared by {})",
		                    name,
		                    formatSize(cache.sizeBytes),
		                    cache.sharingLogicalProcessorCount);
	}
}
#endif // TRIVIAL_PLATFORM_HAS_CPU_AFFINITY

} // namespace

namespace trivial::core {

namespace {

[[nodiscard]] constexpr const char* logLevelName(LogLevel logLevel) noexcept {
	switch (logLevel) {
		case LogLevel::Debug:
			return "debug";
		case LogLevel::Info:
			return "info";
		case LogLevel::Warning:
			return "warning";
		case LogLevel::Error:
			return "error";
		case LogLevel::Fatal:
			return "fatal";
	}

	TRIVIAL_UNREACHABLE();
}

} // namespace

void logMessage(LogLevel level, const char* message) noexcept {
	(void)std::fputs("Trivial ", stderr);
	(void)std::fputs(logLevelName(level), stderr);
	(void)std::fputs(": ", stderr);
	(void)std::fputs(message, stderr);
	(void)std::fputc('\n', stderr);
}

void logMessageWithPrefix(LogLevel level, const char* prefix, const char* message) noexcept {
	(void)std::fputs("Trivial ", stderr);
	(void)std::fputs(logLevelName(level), stderr);
	(void)std::fputs(" [", stderr);
	(void)std::fputs(prefix, stderr);
	(void)std::fputs("]: ", stderr);
	(void)std::fputs(message, stderr);
	(void)std::fputc('\n', stderr);
}

void logOomFailure(const char* prefix, const char* context, std::size_t requestedSize, int osErrorCode) noexcept {
	(void)std::fprintf(stderr, // NOLINT(cppcoreguidelines-pro-type-vararg)
	                   "Trivial %s [%s]: %s (requested %zu bytes, os error %d)\n",
	                   logLevelName(LogLevel::Fatal),
	                   prefix,
	                   context,
	                   requestedSize,
	                   osErrorCode);
}

void logCpuInfo() noexcept {
#if TRIVIAL_ARCH_X86_64
	using Feature = trivial::cpu::X86_64Feature;
#elif TRIVIAL_ARCH_ARM64
	using Feature = trivial::cpu::Arm64Feature;
#endif // Cpu architecture check

	constexpr const char* kPrefix = "CPU";
	const std::string_view kBrand = trivial::cpu::brand();
	const std::string_view kVendor = trivial::cpu::vendor();

	logInfo(kPrefix, "Brand: {}", kBrand.empty() ? "unknown" : kBrand);
	logInfo(kPrefix, "Vendor: {}", kVendor.empty() ? "unknown" : kVendor);
#if TRIVIAL_ARCH_X86_64
	const trivial::cpu::X86_64Info& kInfo = trivial::cpu::architectureInfo();
	logInfo(kPrefix, "Family: {}, model: {}, stepping: {}", kInfo.family, kInfo.model, kInfo.stepping);
	logInfo(kPrefix, "AVX10 version: {}", kInfo.avx10Version != 0 ? std::to_string(kInfo.avx10Version) : "none");
#endif // TRIVIAL_ARCH_X86_64
	logInfo(kPrefix, "Running translated: {}", trivial::cpu::runningTranslated() ? "yes" : "no");
	logInfo(kPrefix, "Logical processors: {}", trivial::cpu::logicalProcessorCount());
	logInfo(kPrefix, "Available logical processors: {}", trivial::cpu::availableLogicalProcessorCount());
	logInfo(kPrefix, "Quota logical processors: {}", trivial::cpu::quotaLogicalProcessorCount());
	logInfo(kPrefix, "Physical cores: {}", trivial::cpu::physicalCoreCount());
	logInfo(kPrefix, "Performance cores: {}", trivial::cpu::performanceCoreCount());
	logInfo(kPrefix, "Efficiency cores: {}", trivial::cpu::efficiencyCoreCount());
	logInfo(kPrefix, "Efficiency classes: {}", trivial::cpu::efficiencyClassCount());

	if constexpr (TRIVIAL_PLATFORM_HAS_CPU_AFFINITY) {
		logInfo(kPrefix, "Efficiency class order: higher is faster");
	}

	logInfo(kPrefix, "Packages: {}", trivial::cpu::packageCount());
	logInfo(kPrefix, "NUMA nodes: {}", trivial::cpu::numaNodeCount());
	logInfo(kPrefix, "Cache line size: {}", formatSize(trivial::cpu::cacheLineSize()));

	constexpr std::string_view kLabel = "Features:";
	std::string line{kLabel};

	for (std::size_t index = 0; index < static_cast<std::size_t>(Feature::Count); ++index) {
		const Feature kFeature = static_cast<Feature>(index);

		if (!trivial::cpu::isDetected(kFeature)) {
			continue;
		}

		const std::string_view kName = trivial::cpu::featureName(kFeature);

		if (line.size() + 1 + kName.size() > TRIVIAL_LOG_SYSTEM_INFO_LINE_WIDTH) {
			logInfo(kPrefix, "{}", line);
			line.assign(kLabel.size(), ' ');
		}

		line += ' ';
		line += kName;
	}

	logInfo(kPrefix, "{}", line.size() == kLabel.size() ? "Features: none" : line);
}

void logCpuTopology() noexcept {
	constexpr std::size_t kL1Data = static_cast<std::size_t>(trivial::cpu::CacheKind::L1Data);
	constexpr std::size_t kL1Instruction = static_cast<std::size_t>(trivial::cpu::CacheKind::L1Instruction);
	constexpr std::size_t kL2 = static_cast<std::size_t>(trivial::cpu::CacheKind::L2);
	constexpr std::size_t kL3 = static_cast<std::size_t>(trivial::cpu::CacheKind::L3);
	constexpr const char* kPrefix = "CPU topology";

#if TRIVIAL_PLATFORM_HAS_CPU_AFFINITY
	const std::span<const trivial::cpu::LogicalProcessorInfo> kLogicalProcessors = trivial::cpu::logicalProcessors();

	if (kLogicalProcessors.empty()) {
		logInfo(kPrefix, "unknown");
		return;
	}

	std::string line;
	std::uint32_t lineCore = 0;

	for (const trivial::cpu::LogicalProcessorInfo& kLogicalProcessor : kLogicalProcessors) {
		if (line.empty() || kLogicalProcessor.core != lineCore) {
			if (!line.empty()) {
				logInfo(kPrefix, "{}", line);
			}

			lineCore = kLogicalProcessor.core;
			line = std::format("Core {}: package {}, class {}",
			                   kLogicalProcessor.core,
			                   kLogicalProcessor.package,
			                   kLogicalProcessor.efficiencyClass);

			if (trivial::cpu::numaNodeCount() > 1) {
				line += std::format(", NUMA node {}", kLogicalProcessor.numaNode);
			}

			appendCache(line, "L1D", kLogicalProcessor.cacheIndices[kL1Data]);
			appendCache(line, "L1I", kLogicalProcessor.cacheIndices[kL1Instruction]);
			appendCache(line, "L2", kLogicalProcessor.cacheIndices[kL2]);
			appendCache(line, "L3", kLogicalProcessor.cacheIndices[kL3]);
			line += ", logical processors:";
		}

		line += std::format(" {}", kLogicalProcessor.processorNumber);
	}

	logInfo(kPrefix, "{}", line);

#else
	const std::span<const trivial::cpu::EfficiencyClassInfo> kClasses = trivial::cpu::efficiencyClasses();

	if (kClasses.empty()) {
		logInfo(kPrefix, "unknown");
		return;
	}

	for (std::size_t efficiencyClass = 0; efficiencyClass < kClasses.size(); ++efficiencyClass) {
		const trivial::cpu::EfficiencyClassInfo& kClass = kClasses[efficiencyClass];
		std::string name = std::format("Class {}", efficiencyClass);

		if (TRIVIAL_PLATFORM_MACOS) {
			if (kClasses.size() == 1) {
				name = "Standard class";
			} else if (kClasses.size() == 2) {
				name = efficiencyClass == 0 ? "Efficiency class" : "Performance class";
			}
		}

		std::string line
		    = std::format("{}: {} cores, {} logical processors", name, kClass.coreCount, kClass.logicalProcessorCount);

		appendCache(line, "L1D", kClass.caches[kL1Data]);
		appendCache(line, "L1I", kClass.caches[kL1Instruction]);
		appendCache(line, "L2", kClass.caches[kL2]);
		appendCache(line, "L3", kClass.caches[kL3]);

		logInfo(kPrefix, "{}", line);
	}
#endif // TRIVIAL_PLATFORM_HAS_CPU_AFFINITY
}

void logMemoryInfo() noexcept {
	constexpr const char* kPrefix = "Memory";

	logInfo(kPrefix, "Page size: {}", formatSize(trivial::memory::pageSize()));
	logInfo(kPrefix, "Allocation granularity: {}", formatSize(trivial::memory::allocationGranularity()));
#if TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
	logInfo(kPrefix, "Large page size: {}", formatSize(trivial::memory::largePageSize()));
#endif // TRIVIAL_MEMORY_ENABLE_LARGE_PAGES
}

void logTimeInfo() noexcept {
	const std::uint64_t kTicksPerSecond = trivial::time::ticksPerSecond();

	if (kTicksPerSecond % TRIVIAL_TIME_HERTZ_PER_MEGAHERTZ == 0) {
		logInfo("Time", "Timer frequency: {} MHz", kTicksPerSecond / TRIVIAL_TIME_HERTZ_PER_MEGAHERTZ);
	} else {
		logInfo("Time", "Timer frequency: {} Hz", kTicksPerSecond);
	}
}

void logSystemInfo() noexcept {
	logCpuInfo();
	logCpuTopology();
	logMemoryInfo();
	logTimeInfo();
}

} // namespace trivial::core

#endif // TRIVIAL_ENABLE_LOGGING

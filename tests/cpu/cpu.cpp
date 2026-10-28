#include <trivial/core/cpu/cpu.h>

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

#include <gtest/gtest.h>

#include <trivial/core/config.h>
#include <trivial/core/platform.h>

#if TRIVIAL_ARCH_X86_64
#include <trivial/core/cpu/cpu_features_x86_64.h>

#elif TRIVIAL_ARCH_ARM64
#include <trivial/core/cpu/cpu_features_arm64.h>

#endif // Cpu architecture check

#if TRIVIAL_PLATFORM_HAS_CPU_AFFINITY
#include <tuple>
#include <vector>
#endif // TRIVIAL_PLATFORM_HAS_CPU_AFFINITY

namespace trivial::cpu {

namespace {

#if TRIVIAL_ARCH_X86_64
using Feature = X86_64Feature;
#elif TRIVIAL_ARCH_ARM64
using Feature = Arm64Feature;
#endif // Cpu architecture check

TEST(CpuTest, GuaranteedFeaturesAreDetected) {
	for (std::size_t index = 0; index < static_cast<std::size_t>(Feature::Count); ++index) {
		const Feature kFeature = static_cast<Feature>(index);

		if (isGuaranteed(kFeature)) {
#if TRIVIAL_ENABLE_LOGGING
			EXPECT_TRUE(isDetected(kFeature)) << featureName(kFeature);
#else
			EXPECT_TRUE(isDetected(kFeature)) << index;
#endif // TRIVIAL_ENABLE_LOGGING
		}
	}
}

TEST(CpuTest, VendorAndBrandAreNullTerminated) {
	EXPECT_NE(std::memchr(vendor(), '\0', sizeof(VendorString)), nullptr);
	EXPECT_NE(std::memchr(brand(), '\0', sizeof(BrandString)), nullptr);
}

TEST(CpuTest, CountsAreConsistent) {
	EXPECT_GT(logicalProcessorCount(), 0U);
	EXPECT_GE(logicalProcessorCount(), physicalCoreCount());
	EXPECT_EQ(performanceCoreCount() + efficiencyCoreCount(), physicalCoreCount());
	EXPECT_GE(efficiencyClassCount(), 1U);
	EXPECT_LE(efficiencyClassCount(), physicalCoreCount());
	EXPECT_GE(packageCount(), 1U);
	EXPECT_GE(numaNodeCount(), 1U);

	if (efficiencyClassCount() == 1) {
		EXPECT_EQ(efficiencyCoreCount(), 0U);
	}
}

TEST(CpuTest, CacheLineSizeIsZeroOrPowerOfTwo) {
	EXPECT_TRUE(cacheLineSize() == 0 || std::has_single_bit(cacheLineSize())) << cacheLineSize();
}

TEST(CpuTest, QuotaAndAvailableAreBounded) {
	const std::uint32_t kAvailable = availableLogicalProcessorCount();
	const std::uint32_t kQuota = quotaLogicalProcessorCount();

	EXPECT_GE(kQuota, 1U);
	EXPECT_LE(kQuota, kAvailable);
	EXPECT_LE(kAvailable, logicalProcessorCount());
}

#if TRIVIAL_PLATFORM_HAS_CPU_AFFINITY
TEST(CpuTest, LogicalProcessorsAreSorted) {
	const std::span<const LogicalProcessorInfo> kLogicalProcessors = logicalProcessors();
	ASSERT_EQ(kLogicalProcessors.size(), logicalProcessorCount());

	for (std::size_t index = 1; index < kLogicalProcessors.size(); ++index) {
		const LogicalProcessorInfo& kPrevious = kLogicalProcessors[index - 1];
		const LogicalProcessorInfo& kCurrent = kLogicalProcessors[index];

		EXPECT_LT(std::tie(kPrevious.efficiencyClass, kPrevious.core, kPrevious.processorNumber),
		          std::tie(kCurrent.efficiencyClass, kCurrent.core, kCurrent.processorNumber))
		    << index;
	}
}

TEST(CpuTest, LogicalProcessorIdsAreInRange) {
	for (const LogicalProcessorInfo& kLogicalProcessor : logicalProcessors()) {
		EXPECT_LT(kLogicalProcessor.core, physicalCoreCount());
		EXPECT_LT(kLogicalProcessor.package, packageCount());
		EXPECT_LT(kLogicalProcessor.numaNode, numaNodeCount());
		EXPECT_LT(kLogicalProcessor.efficiencyClass, efficiencyClassCount());
	}
}

TEST(CpuTest, CacheIndicesMatchCaches) {
	const std::span<const CacheInfo> kCaches = caches();
	std::vector<std::uint32_t> sharers(kCaches.size());

	for (const LogicalProcessorInfo& kLogicalProcessor : logicalProcessors()) {
		for (std::size_t kind = 0; kind < kLogicalProcessor.cacheIndices.size(); ++kind) {
			const std::uint16_t kIndex = kLogicalProcessor.cacheIndices[kind];

			if (kIndex == TRIVIAL_CPU_INVALID_CACHE_INDEX) {
				continue;
			}

			ASSERT_LT(kIndex, kCaches.size());
			EXPECT_EQ(kCaches[kIndex].kind, static_cast<CacheKind>(kind));
			++sharers[kIndex];
		}
	}

	for (std::size_t index = 0; index < kCaches.size(); ++index) {
		EXPECT_EQ(sharers[index], kCaches[index].sharingLogicalProcessorCount) << index;
	}
}

#else
TEST(CpuTest, EfficiencyClassesSumToTotals) {
	const std::span<const EfficiencyClassInfo> kClasses = efficiencyClasses();
	ASSERT_EQ(kClasses.size(), efficiencyClassCount());
	ASSERT_FALSE(kClasses.empty());

	std::uint32_t logicalProcessorTotal = 0;
	std::uint32_t coreTotal = 0;

	for (const EfficiencyClassInfo& kClass : kClasses) {
		logicalProcessorTotal += kClass.logicalProcessorCount;
		coreTotal += kClass.coreCount;
	}

	EXPECT_EQ(logicalProcessorTotal, logicalProcessorCount());
	EXPECT_EQ(coreTotal, physicalCoreCount());
	EXPECT_EQ(kClasses.back().coreCount, performanceCoreCount());
}

TEST(CpuTest, EfficiencyClassCoresAreUniform) {
	for (const EfficiencyClassInfo& kClass : efficiencyClasses()) {
		ASSERT_GT(kClass.coreCount, 0U);
		EXPECT_GE(kClass.logicalProcessorCount, kClass.coreCount);
		EXPECT_EQ(kClass.logicalProcessorCount % kClass.coreCount, 0U);
	}
}

TEST(CpuTest, EfficiencyClassCachesMatchKind) {
	for (const EfficiencyClassInfo& kClass : efficiencyClasses()) {
		for (std::size_t kind = 0; kind < kClass.caches.size(); ++kind) {
			const CacheInfo& kCache = kClass.caches[kind];

			EXPECT_EQ(kCache.kind, static_cast<CacheKind>(kind));
			EXPECT_LE(kCache.sharingLogicalProcessorCount, kClass.logicalProcessorCount);
			EXPECT_EQ(kCache.sizeBytes == 0, kCache.sharingLogicalProcessorCount == 0) << kind;
		}
	}
}

#endif // TRIVIAL_PLATFORM_HAS_CPU_AFFINITY

} // namespace

} // namespace trivial::cpu

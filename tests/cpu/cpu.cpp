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

TEST(CpuTest, TargetFeaturesAreDetected) {
	for (std::size_t i = 0; i < static_cast<std::size_t>(Feature::Count); ++i) {
		const Feature kFeature = static_cast<Feature>(i);

		if (isTargetFeature(kFeature)) {
#if TRIVIAL_ENABLE_LOGGING
			EXPECT_TRUE(isDetected(kFeature)) << featureName(kFeature);
#else
			EXPECT_TRUE(isDetected(kFeature)) << i;
#endif // TRIVIAL_ENABLE_LOGGING
		}
	}
}

TEST(CpuTest, VendorAndBrandAreNullTerminated) {
	EXPECT_NE(std::memchr(vendor(), '\0', sizeof(VendorString)), nullptr);
	EXPECT_NE(std::memchr(brand(), '\0', sizeof(BrandString)), nullptr);
}

TEST(CpuTest, CountsAreConsistent) {
	if (logicalProcessorCount() == 0) {
		EXPECT_EQ(physicalCoreCount(), 0U);
		EXPECT_EQ(efficiencyClassCount(), 0U);
		EXPECT_EQ(packageCount(), 0U);
		EXPECT_EQ(numaNodeCount(), 0U);
		GTEST_SKIP() << "The topology is unknown on this platform";
	}

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

	if (logicalProcessorCount() != 0) {
		EXPECT_LE(kAvailable, logicalProcessorCount());
	}
}

#if TRIVIAL_ARCH_X86_64
TEST(CpuTest, IdentityIsReported) {
	EXPECT_NE(*vendor(), '\0');
	EXPECT_NE(*brand(), ' ');
	EXPECT_NE(architectureInfo().family, 0U);
}

TEST(CpuTest, Avx10VersionMatchesDetection) {
	EXPECT_EQ(architectureInfo().avx10Version >= 1, isDetected(X86_64Feature::Avx10V1));
	EXPECT_EQ(architectureInfo().avx10Version >= 2, isDetected(X86_64Feature::Avx10V2));
}
#endif // Cpu architecture check

#if TRIVIAL_PLATFORM_HAS_CPU_AFFINITY
TEST(CpuTest, LogicalProcessorsAreSorted) {
	const std::span<const LogicalProcessorInfo> kLogicalProcessors = logicalProcessors();
	ASSERT_EQ(kLogicalProcessors.size(), logicalProcessorCount());

	for (std::size_t i = 1; i < kLogicalProcessors.size(); ++i) {
		const LogicalProcessorInfo& kPrevious = kLogicalProcessors[i - 1];
		const LogicalProcessorInfo& kCurrent = kLogicalProcessors[i];

		EXPECT_LT(std::tie(kPrevious.efficiencyClass, kPrevious.core, kPrevious.processorNumber),
		          std::tie(kCurrent.efficiencyClass, kCurrent.core, kCurrent.processorNumber))
		    << i;
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
		for (std::size_t i = 0; i < kLogicalProcessor.cacheIndices.size(); ++i) {
			const std::uint16_t kCacheIndex = kLogicalProcessor.cacheIndices[i];

			if (kCacheIndex == TRIVIAL_CPU_INVALID_CACHE_INDEX) {
				continue;
			}

			ASSERT_LT(kCacheIndex, kCaches.size());
			EXPECT_EQ(kCaches[kCacheIndex].kind, static_cast<CacheKind>(i));
			++sharers[kCacheIndex];
		}
	}

	for (std::size_t i = 0; i < kCaches.size(); ++i) {
		EXPECT_EQ(sharers[i], kCaches[i].sharingLogicalProcessorCount) << i;
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
		for (std::size_t i = 0; i < kClass.caches.size(); ++i) {
			const CacheInfo& kCache = kClass.caches[i];

			EXPECT_EQ(kCache.kind, static_cast<CacheKind>(i));
			EXPECT_LE(kCache.sharingLogicalProcessorCount, kClass.logicalProcessorCount);
			EXPECT_EQ(kCache.sizeBytes == 0, kCache.sharingLogicalProcessorCount == 0) << i;
		}
	}
}

#endif // TRIVIAL_PLATFORM_HAS_CPU_AFFINITY

} // namespace

} // namespace trivial::cpu

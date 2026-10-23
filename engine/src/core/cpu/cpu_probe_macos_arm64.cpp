#include "core/cpu/cpu_probe.h"

#include <arm/cpu_capabilities_public.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <sys/sysctl.h>

#include <trivial/core/assert.h>
#include <trivial/core/cpu/cpu.h>
#include <trivial/core/cpu/cpu_feature_set.h>
#include <trivial/core/cpu/cpu_features_arm64.h>

#define TRIVIAL_CPU_MACOS_CAPS_BYTE_BITS static_cast<std::uint32_t>(std::numeric_limits<std::uint8_t>::digits)

namespace {

struct FeatureSource {
	trivial::cpu::Arm64Feature feature;
	std::uint32_t capsBit;
	const char* name;
};

constexpr std::array g_kFeatureSources{
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Crc32,
        .capsBit = CAP_BIT_FEAT_CRC32,
        .name = "hw.optional.armv8_crc32",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Aes,
        .capsBit = CAP_BIT_FEAT_AES,
        .name = "hw.optional.arm.FEAT_AES",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Pmull,
        .capsBit = CAP_BIT_FEAT_PMULL,
        .name = "hw.optional.arm.FEAT_PMULL",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Sha1,
        .capsBit = CAP_BIT_FEAT_SHA1,
        .name = "hw.optional.arm.FEAT_SHA1",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Sha256,
        .capsBit = CAP_BIT_FEAT_SHA256,
        .name = "hw.optional.arm.FEAT_SHA256",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Sha3,
        .capsBit = CAP_BIT_FEAT_SHA3,
        .name = "hw.optional.arm.FEAT_SHA3",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Sha512,
        .capsBit = CAP_BIT_FEAT_SHA512,
        .name = "hw.optional.arm.FEAT_SHA512",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Lse,
        .capsBit = CAP_BIT_FEAT_LSE,
        .name = "hw.optional.arm.FEAT_LSE",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Lse2,
        .capsBit = CAP_BIT_FEAT_LSE2,
        .name = "hw.optional.arm.FEAT_LSE2",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Rdm,
        .capsBit = CAP_BIT_FEAT_RDM,
        .name = "hw.optional.arm.FEAT_RDM",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Lrcpc,
        .capsBit = CAP_BIT_FEAT_LRCPC,
        .name = "hw.optional.arm.FEAT_LRCPC",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Lrcpc2,
        .capsBit = CAP_BIT_FEAT_LRCPC2,
        .name = "hw.optional.arm.FEAT_LRCPC2",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::DotProd,
        .capsBit = CAP_BIT_FEAT_DotProd,
        .name = "hw.optional.arm.FEAT_DotProd",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Fp16,
        .capsBit = CAP_BIT_FEAT_FP16,
        .name = "hw.optional.arm.FEAT_FP16",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Fhm,
        .capsBit = CAP_BIT_FEAT_FHM,
        .name = "hw.optional.arm.FEAT_FHM",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Jscvt,
        .capsBit = CAP_BIT_FEAT_JSCVT,
        .name = "hw.optional.arm.FEAT_JSCVT",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Fcma,
        .capsBit = CAP_BIT_FEAT_FCMA,
        .name = "hw.optional.arm.FEAT_FCMA",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Frintts,
        .capsBit = CAP_BIT_FEAT_FRINTTS,
        .name = "hw.optional.arm.FEAT_FRINTTS",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::FlagM,
        .capsBit = CAP_BIT_FEAT_FlagM,
        .name = "hw.optional.arm.FEAT_FlagM",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::FlagM2,
        .capsBit = CAP_BIT_FEAT_FlagM2,
        .name = "hw.optional.arm.FEAT_FlagM2",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Sb,
        .capsBit = CAP_BIT_FEAT_SB,
        .name = "hw.optional.arm.FEAT_SB",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Ssbs,
        .capsBit = CAP_BIT_FEAT_SSBS,
        .name = "hw.optional.arm.FEAT_SSBS",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Dit,
        .capsBit = CAP_BIT_FEAT_DIT,
        .name = "hw.optional.arm.FEAT_DIT",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Dpb,
        .capsBit = CAP_BIT_FEAT_DPB,
        .name = "hw.optional.arm.FEAT_DPB",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Dpb2,
        .capsBit = CAP_BIT_FEAT_DPB2,
        .name = "hw.optional.arm.FEAT_DPB2",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Bti,
        .capsBit = CAP_BIT_FEAT_BTI,
        .name = "hw.optional.arm.FEAT_BTI",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::PAuth,
        .capsBit = CAP_BIT_FEAT_PAuth,
        .name = "hw.optional.arm.FEAT_PAuth",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::I8mm,
        .capsBit = CAP_BIT_FEAT_I8MM,
        .name = "hw.optional.arm.FEAT_I8MM",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Bf16,
        .capsBit = CAP_BIT_FEAT_BF16,
        .name = "hw.optional.arm.FEAT_BF16",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Ebf16,
        .capsBit = CAP_BIT_FEAT_EBF16,
        .name = "hw.optional.arm.FEAT_EBF16",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Mte,
        .capsBit = CAP_BIT_FEAT_MTE,
        .name = "hw.optional.arm.FEAT_MTE",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Sme,
        .capsBit = CAP_BIT_FEAT_SME,
        .name = "hw.optional.arm.FEAT_SME",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Sme2,
        .capsBit = CAP_BIT_FEAT_SME2,
        .name = "hw.optional.arm.FEAT_SME2",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::SmeF64F64,
        .capsBit = CAP_BIT_FEAT_SME_F64F64,
        .name = "hw.optional.arm.FEAT_SME_F64F64",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::SmeI16I64,
        .capsBit = CAP_BIT_FEAT_SME_I16I64,
        .name = "hw.optional.arm.FEAT_SME_I16I64",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Cssc,
        .capsBit = CAP_BIT_FEAT_CSSC,
        .name = "hw.optional.arm.FEAT_CSSC",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Wfxt,
        .capsBit = CAP_BIT_FEAT_WFxT,
        .name = "hw.optional.arm.FEAT_WFxT",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Hbc,
        .capsBit = CAP_BIT_FEAT_HBC,
        .name = "hw.optional.arm.FEAT_HBC",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Afp,
        .capsBit = CAP_BIT_FEAT_AFP,
        .name = "hw.optional.arm.FEAT_AFP",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Rpres,
        .capsBit = CAP_BIT_FEAT_RPRES,
        .name = "hw.optional.arm.FEAT_RPRES",
    },
    FeatureSource{
        .feature = trivial::cpu::Arm64Feature::Ecv,
        .capsBit = CAP_BIT_FEAT_ECV,
        .name = "hw.optional.arm.FEAT_ECV",
    },
};

[[nodiscard]] bool readSysctlFlag(const char* name) noexcept {
	int value = 0;
	std::size_t size = sizeof(value);
	return sysctlbyname(name, &value, &size, nullptr, 0) == 0 && value != 0;
}

[[nodiscard]] bool isCapsBitSet(std::span<const std::uint8_t> caps, std::uint32_t bit) noexcept {
	const std::size_t kByte = bit / TRIVIAL_CPU_MACOS_CAPS_BYTE_BITS;
	const std::uint32_t kMask = 1U << (bit % TRIVIAL_CPU_MACOS_CAPS_BYTE_BITS);
	return kByte < caps.size() && (caps[kByte] & kMask) != 0;
}

void probeFeatures() noexcept {
	std::array<std::uint8_t, (CAP_BIT_NB + TRIVIAL_CPU_MACOS_CAPS_BYTE_BITS - 1) / TRIVIAL_CPU_MACOS_CAPS_BYTE_BITS>
	    capsBuffer{};
	std::size_t capsSize = capsBuffer.size();
	const bool kHasCaps = sysctlbyname("hw.optional.arm.caps", capsBuffer.data(), &capsSize, nullptr, 0) == 0;
	const std::span<const std::uint8_t> kCaps{capsBuffer.data(), capsSize};

	for (const FeatureSource& kSource : g_kFeatureSources) {
		const bool kPresent = kHasCaps ? isCapsBitSet(kCaps, kSource.capsBit) : readSysctlFlag(kSource.name);
		TRIVIAL_ASSERT(kPresent == readSysctlFlag(kSource.name));

		if (kPresent) {
			trivial::cpu::detail::g_detectedCapabilities.identity.features.insert(kSource.feature);
		}
	}
}

} // namespace

namespace trivial::cpu {

void probeIdentity() noexcept {
	probeFeatures();

	std::memcpy(detail::g_detectedCapabilities.identity.vendor.data(), "Apple", sizeof("Apple"));

	BrandString& brand = detail::g_detectedCapabilities.identity.brand;
	std::size_t brandSize = brand.size();
	(void)sysctlbyname("machdep.cpu.brand_string", brand.data(), &brandSize, nullptr, 0);
	brand.back() = '\0';
}

} // namespace trivial::cpu

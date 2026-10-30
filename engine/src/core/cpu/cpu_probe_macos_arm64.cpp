#include "core/cpu/cpu_probe.h"

#include <arm/cpu_capabilities_public.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <sys/sysctl.h>
#include <vector>

#include <trivial/core/assert.h>
#include <trivial/core/cpu/cpu.h>
#include <trivial/core/cpu/cpu_feature_set.h>
#include <trivial/core/cpu/cpu_features_arm64.h>

#include "core/cpu/cpu_probe_arm64.h"

#define TRIVIAL_CPU_MACOS_NO_CAPS_BIT std::numeric_limits<std::uint32_t>::max()
#define TRIVIAL_CPU_MACOS_CAPS_BYTE_BITS static_cast<std::uint32_t>(std::numeric_limits<std::uint8_t>::digits)

namespace {

struct SysctlSource {
	std::uint32_t capsBit;
	const char* name;
};

[[nodiscard]] constexpr SysctlSource sysctlSource(trivial::cpu::Arm64Feature feature) noexcept {
	switch (feature) {
		case trivial::cpu::Arm64Feature::Lse:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_LSE, .name = "hw.optional.arm.FEAT_LSE"};

		case trivial::cpu::Arm64Feature::Lse2:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_LSE2, .name = "hw.optional.arm.FEAT_LSE2"};

		case trivial::cpu::Arm64Feature::Lrcpc:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_LRCPC, .name = "hw.optional.arm.FEAT_LRCPC"};

		case trivial::cpu::Arm64Feature::Lrcpc2:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_LRCPC2, .name = "hw.optional.arm.FEAT_LRCPC2"};

		case trivial::cpu::Arm64Feature::Wfxt:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_WFxT, .name = "hw.optional.arm.FEAT_WFxT"};

		case trivial::cpu::Arm64Feature::Crc32:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_CRC32, .name = "hw.optional.armv8_crc32"};

		case trivial::cpu::Arm64Feature::Aes:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_AES, .name = "hw.optional.arm.FEAT_AES"};

		case trivial::cpu::Arm64Feature::Pmull:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_PMULL, .name = "hw.optional.arm.FEAT_PMULL"};

		case trivial::cpu::Arm64Feature::Sha1:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_SHA1, .name = "hw.optional.arm.FEAT_SHA1"};

		case trivial::cpu::Arm64Feature::Sha256:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_SHA256, .name = "hw.optional.arm.FEAT_SHA256"};

		case trivial::cpu::Arm64Feature::Sha3:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_SHA3, .name = "hw.optional.arm.FEAT_SHA3"};

		case trivial::cpu::Arm64Feature::Sha512:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_SHA512, .name = "hw.optional.arm.FEAT_SHA512"};

		case trivial::cpu::Arm64Feature::Cssc:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_CSSC, .name = "hw.optional.arm.FEAT_CSSC"};

		case trivial::cpu::Arm64Feature::FlagM:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_FlagM, .name = "hw.optional.arm.FEAT_FlagM"};

		case trivial::cpu::Arm64Feature::FlagM2:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_FlagM2, .name = "hw.optional.arm.FEAT_FlagM2"};

		case trivial::cpu::Arm64Feature::Jscvt:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_JSCVT, .name = "hw.optional.arm.FEAT_JSCVT"};

		case trivial::cpu::Arm64Feature::Hbc:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_HBC, .name = "hw.optional.arm.FEAT_HBC"};

		case trivial::cpu::Arm64Feature::Frintts:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_FRINTTS, .name = "hw.optional.arm.FEAT_FRINTTS"};

		case trivial::cpu::Arm64Feature::Afp:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_AFP, .name = "hw.optional.arm.FEAT_AFP"};

		case trivial::cpu::Arm64Feature::Rpres:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_RPRES, .name = "hw.optional.arm.FEAT_RPRES"};

		case trivial::cpu::Arm64Feature::FpSyncExceptions:
			return SysctlSource{.capsBit = CAP_BIT_FP_SyncExceptions, .name = "hw.optional.arm.FP_SyncExceptions"};

		case trivial::cpu::Arm64Feature::Rdm:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_RDM, .name = "hw.optional.arm.FEAT_RDM"};

		case trivial::cpu::Arm64Feature::DotProd:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_DotProd, .name = "hw.optional.arm.FEAT_DotProd"};

		case trivial::cpu::Arm64Feature::Fp16:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_FP16, .name = "hw.optional.arm.FEAT_FP16"};

		case trivial::cpu::Arm64Feature::Fhm:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_FHM, .name = "hw.optional.arm.FEAT_FHM"};

		case trivial::cpu::Arm64Feature::Fcma:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_FCMA, .name = "hw.optional.arm.FEAT_FCMA"};

		case trivial::cpu::Arm64Feature::I8mm:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_I8MM, .name = "hw.optional.arm.FEAT_I8MM"};

		case trivial::cpu::Arm64Feature::Bf16:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_BF16, .name = "hw.optional.arm.FEAT_BF16"};

		case trivial::cpu::Arm64Feature::Ebf16:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_EBF16, .name = "hw.optional.arm.FEAT_EBF16"};

		case trivial::cpu::Arm64Feature::Lut:
			return SysctlSource{.capsBit = TRIVIAL_CPU_MACOS_NO_CAPS_BIT, .name = "hw.optional.arm.FEAT_LUT"};

		case trivial::cpu::Arm64Feature::Faminmax:
			return SysctlSource{.capsBit = TRIVIAL_CPU_MACOS_NO_CAPS_BIT, .name = "hw.optional.arm.FEAT_FAMINMAX"};

		case trivial::cpu::Arm64Feature::Fpmr:
			return SysctlSource{.capsBit = TRIVIAL_CPU_MACOS_NO_CAPS_BIT, .name = "hw.optional.arm.FEAT_FPMR"};

		case trivial::cpu::Arm64Feature::Fp8:
			return SysctlSource{.capsBit = TRIVIAL_CPU_MACOS_NO_CAPS_BIT, .name = "hw.optional.arm.FEAT_FP8"};

		case trivial::cpu::Arm64Feature::SveB16b16:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_SVE_B16B16, .name = "hw.optional.arm.FEAT_SVE_B16B16"};

		case trivial::cpu::Arm64Feature::Sme:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_SME, .name = "hw.optional.arm.FEAT_SME"};

		case trivial::cpu::Arm64Feature::Sme2:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_SME2, .name = "hw.optional.arm.FEAT_SME2"};

		case trivial::cpu::Arm64Feature::Sme2p1:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_SME2p1, .name = "hw.optional.arm.FEAT_SME2p1"};

		case trivial::cpu::Arm64Feature::SmeF64F64:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_SME_F64F64, .name = "hw.optional.arm.FEAT_SME_F64F64"};

		case trivial::cpu::Arm64Feature::SmeI16I64:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_SME_I16I64, .name = "hw.optional.arm.FEAT_SME_I16I64"};

		case trivial::cpu::Arm64Feature::SmeF16F16:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_SME_F16F16, .name = "hw.optional.arm.FEAT_SME_F16F16"};

		case trivial::cpu::Arm64Feature::SmeB16B16:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_SME_B16B16, .name = "hw.optional.arm.FEAT_SME_B16B16"};

		case trivial::cpu::Arm64Feature::SmeF8F16:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_SME_F8F16, .name = "hw.optional.arm.FEAT_SME_F8F16"};

		case trivial::cpu::Arm64Feature::SmeF8F32:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_SME_F8F32, .name = "hw.optional.arm.FEAT_SME_F8F32"};

		case trivial::cpu::Arm64Feature::SmeLutv2:
			return SysctlSource{.capsBit = TRIVIAL_CPU_MACOS_NO_CAPS_BIT, .name = "hw.optional.arm.FEAT_SME_LUTv2"};

		case trivial::cpu::Arm64Feature::Mte:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_MTE, .name = "hw.optional.arm.FEAT_MTE"};

		case trivial::cpu::Arm64Feature::Mte2:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_MTE2, .name = "hw.optional.arm.FEAT_MTE2"};

		case trivial::cpu::Arm64Feature::Mte3:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_MTE3, .name = "hw.optional.arm.FEAT_MTE3"};

		case trivial::cpu::Arm64Feature::MteAsync:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_MTE_ASYNC, .name = "hw.optional.arm.FEAT_MTE_ASYNC"};

		case trivial::cpu::Arm64Feature::MteStoreOnly:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_MTE_STORE_ONLY, .name = "hw.optional.arm.FEAT_MTE_STORE_ONLY"};

		case trivial::cpu::Arm64Feature::Sb:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_SB, .name = "hw.optional.arm.FEAT_SB"};

		case trivial::cpu::Arm64Feature::Ssbs:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_SSBS, .name = "hw.optional.arm.FEAT_SSBS"};

		case trivial::cpu::Arm64Feature::Dit:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_DIT, .name = "hw.optional.arm.FEAT_DIT"};

		case trivial::cpu::Arm64Feature::Bti:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_BTI, .name = "hw.optional.arm.FEAT_BTI"};

		case trivial::cpu::Arm64Feature::PAuth:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_PAuth, .name = "hw.optional.arm.FEAT_PAuth"};

		case trivial::cpu::Arm64Feature::Dpb:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_DPB, .name = "hw.optional.arm.FEAT_DPB"};

		case trivial::cpu::Arm64Feature::Dpb2:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_DPB2, .name = "hw.optional.arm.FEAT_DPB2"};

		case trivial::cpu::Arm64Feature::Ecv:
			return SysctlSource{.capsBit = CAP_BIT_FEAT_ECV, .name = "hw.optional.arm.FEAT_ECV"};

		case trivial::cpu::Arm64Feature::Lse128:
		case trivial::cpu::Arm64Feature::Lsfe:
		case trivial::cpu::Arm64Feature::Lrcpc3:
		case trivial::cpu::Arm64Feature::Mops:
		case trivial::cpu::Arm64Feature::Rprfm:
		case trivial::cpu::Arm64Feature::Sm3:
		case trivial::cpu::Arm64Feature::Sm4:
		case trivial::cpu::Arm64Feature::Rng:
		case trivial::cpu::Arm64Feature::Cmpbr:
		case trivial::cpu::Arm64Feature::Fprcvt:
		case trivial::cpu::Arm64Feature::F16f32dot:
		case trivial::cpu::Arm64Feature::F16f32mm:
		case trivial::cpu::Arm64Feature::F16mm:
		case trivial::cpu::Arm64Feature::Fp8Fma:
		case trivial::cpu::Arm64Feature::Fp8Dot4:
		case trivial::cpu::Arm64Feature::Fp8Dot2:
		case trivial::cpu::Arm64Feature::F8f16mm:
		case trivial::cpu::Arm64Feature::F8f32mm:
		case trivial::cpu::Arm64Feature::Sve:
		case trivial::cpu::Arm64Feature::Sve2:
		case trivial::cpu::Arm64Feature::Sve2p1:
		case trivial::cpu::Arm64Feature::Sve2p2:
		case trivial::cpu::Arm64Feature::Sve2p3:
		case trivial::cpu::Arm64Feature::SveI8mm:
		case trivial::cpu::Arm64Feature::SveBf16:
		case trivial::cpu::Arm64Feature::SveEbf16:
		case trivial::cpu::Arm64Feature::SveF32mm:
		case trivial::cpu::Arm64Feature::SveF64mm:
		case trivial::cpu::Arm64Feature::SveF16f32mm:
		case trivial::cpu::Arm64Feature::SveB16mm:
		case trivial::cpu::Arm64Feature::SveBfscale:
		case trivial::cpu::Arm64Feature::SveAes:
		case trivial::cpu::Arm64Feature::SvePmull128:
		case trivial::cpu::Arm64Feature::SveAes2:
		case trivial::cpu::Arm64Feature::SveBitperm:
		case trivial::cpu::Arm64Feature::SveSha3:
		case trivial::cpu::Arm64Feature::SveSm4:
		case trivial::cpu::Arm64Feature::SvePerm256:
		case trivial::cpu::Arm64Feature::SveLut6:
		case trivial::cpu::Arm64Feature::Sme2p2:
		case trivial::cpu::Arm64Feature::Sme2p3:
		case trivial::cpu::Arm64Feature::SmeLut6:
		case trivial::cpu::Arm64Feature::SmeTmop:
		case trivial::cpu::Arm64Feature::SmeMop4:
		case trivial::cpu::Arm64Feature::SmeFa64:
		case trivial::cpu::Arm64Feature::SsveAes:
		case trivial::cpu::Arm64Feature::SsveBitperm:
		case trivial::cpu::Arm64Feature::SsveFexpa:
		case trivial::cpu::Arm64Feature::SsveFp8Fma:
		case trivial::cpu::Arm64Feature::SsveFp8Dot4:
		case trivial::cpu::Arm64Feature::SsveFp8Dot2:
		case trivial::cpu::Arm64Feature::SsveF8f16mm:
		case trivial::cpu::Arm64Feature::SsveF8f32mm:
		case trivial::cpu::Arm64Feature::MteFar:
		case trivial::cpu::Arm64Feature::Poe:
		case trivial::cpu::Arm64Feature::Gcs:
			return SysctlSource{.capsBit = TRIVIAL_CPU_MACOS_NO_CAPS_BIT, .name = nullptr};
		case trivial::cpu::Arm64Feature::Count:
			break;
	}

	TRIVIAL_UNREACHABLE();
}

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

	trivial::cpu::FeatureSet<trivial::cpu::Arm64Feature> reported;

	for (std::size_t i = 0; i < static_cast<std::size_t>(trivial::cpu::Arm64Feature::Count); ++i) {
		const trivial::cpu::Arm64Feature kFeature = static_cast<trivial::cpu::Arm64Feature>(i);
		const SysctlSource kSource = sysctlSource(kFeature);

		if (kSource.name == nullptr) {
			continue;
		}

		const bool kPresent = kHasCaps && kSource.capsBit != TRIVIAL_CPU_MACOS_NO_CAPS_BIT
		                          ? isCapsBitSet(kCaps, kSource.capsBit)
		                          : readSysctlFlag(kSource.name);
		TRIVIAL_ASSERT(kPresent == readSysctlFlag(kSource.name));

		if (kPresent) {
			reported.insert(kFeature);
		}
	}

	trivial::cpu::insertFeaturesWithPrerequisites<trivial::cpu::featurePrerequisites>(reported);
}

} // namespace

namespace trivial::cpu {

void probeIdentity() noexcept {
	probeFeatures();

	std::memcpy(detail::g_detectedCapabilities.identity.vendor.data(), "Apple", sizeof("Apple"));

	std::size_t brandSize = 0;

	if (sysctlbyname("machdep.cpu.brand_string", nullptr, &brandSize, nullptr, 0) != 0) {
		return;
	}

	std::vector<char> fullBrand(brandSize);

	if (sysctlbyname("machdep.cpu.brand_string", fullBrand.data(), &brandSize, nullptr, 0) != 0) {
		return;
	}

	copyBrand({fullBrand.data(), brandSize});
}

} // namespace trivial::cpu

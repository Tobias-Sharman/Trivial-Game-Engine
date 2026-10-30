#include "core/cpu/cpu_probe.h"

#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <string_view>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX

#include <windows.h> // IWYU pragma: keep

#include <trivial/core/assert.h>
#include <trivial/core/cpu/cpu.h>
#include <trivial/core/cpu/cpu_feature_set.h>
#include <trivial/core/cpu/cpu_features_arm64.h>
#include <trivial/core/log.h>

#include "core/cpu/cpu_probe_arm64.h"

#define TRIVIAL_CPU_WINDOWS_ARM64_PROCESSOR_KEY_CAPACITY std::size_t{64}

#define TRIVIAL_CPU_WINDOWS_ARM64_PF_V8_CRYPTO DWORD{30}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_V8_CRC32 DWORD{31}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_V81_ATOMIC DWORD{34}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_V82_DP DWORD{43}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_V83_JSCVT DWORD{44}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_V83_LRCPC DWORD{45}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SVE DWORD{46}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SVE2 DWORD{47}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SVE2_1 DWORD{48}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SVE_AES DWORD{49}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SVE_PMULL128 DWORD{50}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SVE_BITPERM DWORD{51}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SVE_BF16 DWORD{52}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SVE_EBF16 DWORD{53}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SVE_B16B16 DWORD{54}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SVE_SHA3 DWORD{55}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SVE_SM4 DWORD{56}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SVE_I8MM DWORD{57}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SVE_F32MM DWORD{58}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SVE_F64MM DWORD{59}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_LSE2 DWORD{62}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SHA3 DWORD{64}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SHA512 DWORD{65}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_V82_I8MM DWORD{66}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_V82_FP16 DWORD{67}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_V86_BF16 DWORD{68}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_V86_EBF16 DWORD{69}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SME DWORD{70}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SME2 DWORD{71}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SME2_1 DWORD{72}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SME2_2 DWORD{73}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SME_AES DWORD{74}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SME_SBITPERM DWORD{75}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SME_SF8MM4 DWORD{76}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SME_SF8MM8 DWORD{77}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SME_SF8DP2 DWORD{78}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SME_SF8DP4 DWORD{79}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SME_SF8FMA DWORD{80}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SME_F8F32 DWORD{81}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SME_F8F16 DWORD{82}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SME_F16F16 DWORD{83}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SME_B16B16 DWORD{84}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SME_F64F64 DWORD{85}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SME_I16I64 DWORD{86}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SME_LUTV2 DWORD{87}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_SME_FA64 DWORD{88}
#define TRIVIAL_CPU_WINDOWS_ARM64_PF_NONE ~DWORD{0}

namespace {

[[nodiscard]] constexpr DWORD processorFeature(trivial::cpu::Arm64Feature feature) noexcept {
	switch (feature) {
		case trivial::cpu::Arm64Feature::Aes:
		case trivial::cpu::Arm64Feature::Pmull:
		case trivial::cpu::Arm64Feature::Sha1:
		case trivial::cpu::Arm64Feature::Sha256:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_V8_CRYPTO;

		case trivial::cpu::Arm64Feature::Crc32:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_V8_CRC32;

		case trivial::cpu::Arm64Feature::Lse:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_V81_ATOMIC;

		case trivial::cpu::Arm64Feature::DotProd:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_V82_DP;

		case trivial::cpu::Arm64Feature::Jscvt:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_V83_JSCVT;

		case trivial::cpu::Arm64Feature::Lrcpc:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_V83_LRCPC;

		case trivial::cpu::Arm64Feature::Sve:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SVE;

		case trivial::cpu::Arm64Feature::Sve2:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SVE2;

		case trivial::cpu::Arm64Feature::Sve2p1:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SVE2_1;

		case trivial::cpu::Arm64Feature::SveAes:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SVE_AES;

		case trivial::cpu::Arm64Feature::SvePmull128:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SVE_PMULL128;

		case trivial::cpu::Arm64Feature::SveBitperm:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SVE_BITPERM;

		case trivial::cpu::Arm64Feature::SveBf16:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SVE_BF16;

		case trivial::cpu::Arm64Feature::SveEbf16:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SVE_EBF16;

		case trivial::cpu::Arm64Feature::SveB16b16:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SVE_B16B16;

		case trivial::cpu::Arm64Feature::SveSha3:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SVE_SHA3;

		case trivial::cpu::Arm64Feature::SveSm4:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SVE_SM4;

		case trivial::cpu::Arm64Feature::SveI8mm:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SVE_I8MM;

		case trivial::cpu::Arm64Feature::SveF32mm:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SVE_F32MM;

		case trivial::cpu::Arm64Feature::SveF64mm:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SVE_F64MM;

		case trivial::cpu::Arm64Feature::Lse2:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_LSE2;

		case trivial::cpu::Arm64Feature::Sha3:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SHA3;

		case trivial::cpu::Arm64Feature::Sha512:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SHA512;

		case trivial::cpu::Arm64Feature::I8mm:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_V82_I8MM;

		case trivial::cpu::Arm64Feature::Fp16:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_V82_FP16;

		case trivial::cpu::Arm64Feature::Bf16:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_V86_BF16;

		case trivial::cpu::Arm64Feature::Ebf16:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_V86_EBF16;

		case trivial::cpu::Arm64Feature::Sme:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SME;

		case trivial::cpu::Arm64Feature::Sme2:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SME2;

		case trivial::cpu::Arm64Feature::Sme2p1:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SME2_1;

		case trivial::cpu::Arm64Feature::Sme2p2:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SME2_2;

		case trivial::cpu::Arm64Feature::SsveAes:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SME_AES;

		case trivial::cpu::Arm64Feature::SsveBitperm:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SME_SBITPERM;

		case trivial::cpu::Arm64Feature::SsveF8f16mm:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SME_SF8MM4;

		case trivial::cpu::Arm64Feature::SsveF8f32mm:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SME_SF8MM8;

		case trivial::cpu::Arm64Feature::SsveFp8Dot2:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SME_SF8DP2;

		case trivial::cpu::Arm64Feature::SsveFp8Dot4:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SME_SF8DP4;

		case trivial::cpu::Arm64Feature::SsveFp8Fma:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SME_SF8FMA;

		case trivial::cpu::Arm64Feature::SmeF8F32:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SME_F8F32;

		case trivial::cpu::Arm64Feature::SmeF8F16:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SME_F8F16;

		case trivial::cpu::Arm64Feature::SmeF16F16:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SME_F16F16;

		case trivial::cpu::Arm64Feature::SmeB16B16:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SME_B16B16;

		case trivial::cpu::Arm64Feature::SmeF64F64:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SME_F64F64;

		case trivial::cpu::Arm64Feature::SmeI16I64:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SME_I16I64;

		case trivial::cpu::Arm64Feature::SmeLutv2:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SME_LUTV2;

		case trivial::cpu::Arm64Feature::SmeFa64:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_SME_FA64;

		case trivial::cpu::Arm64Feature::Lse128:
		case trivial::cpu::Arm64Feature::Lsfe:
		case trivial::cpu::Arm64Feature::Lrcpc2:
		case trivial::cpu::Arm64Feature::Lrcpc3:
		case trivial::cpu::Arm64Feature::Mops:
		case trivial::cpu::Arm64Feature::Rprfm:
		case trivial::cpu::Arm64Feature::Wfxt:
		case trivial::cpu::Arm64Feature::Sm3:
		case trivial::cpu::Arm64Feature::Sm4:
		case trivial::cpu::Arm64Feature::Rng:
		case trivial::cpu::Arm64Feature::Cssc:
		case trivial::cpu::Arm64Feature::FlagM:
		case trivial::cpu::Arm64Feature::FlagM2:
		case trivial::cpu::Arm64Feature::Hbc:
		case trivial::cpu::Arm64Feature::Cmpbr:
		case trivial::cpu::Arm64Feature::Frintts:
		case trivial::cpu::Arm64Feature::Fprcvt:
		case trivial::cpu::Arm64Feature::Afp:
		case trivial::cpu::Arm64Feature::Rpres:
		case trivial::cpu::Arm64Feature::FpSyncExceptions:
		case trivial::cpu::Arm64Feature::Rdm:
		case trivial::cpu::Arm64Feature::Fhm:
		case trivial::cpu::Arm64Feature::F16f32dot:
		case trivial::cpu::Arm64Feature::F16f32mm:
		case trivial::cpu::Arm64Feature::F16mm:
		case trivial::cpu::Arm64Feature::Fcma:
		case trivial::cpu::Arm64Feature::Lut:
		case trivial::cpu::Arm64Feature::Faminmax:
		case trivial::cpu::Arm64Feature::Fpmr:
		case trivial::cpu::Arm64Feature::Fp8:
		case trivial::cpu::Arm64Feature::Fp8Fma:
		case trivial::cpu::Arm64Feature::Fp8Dot4:
		case trivial::cpu::Arm64Feature::Fp8Dot2:
		case trivial::cpu::Arm64Feature::F8f16mm:
		case trivial::cpu::Arm64Feature::F8f32mm:
		case trivial::cpu::Arm64Feature::Sve2p2:
		case trivial::cpu::Arm64Feature::Sve2p3:
		case trivial::cpu::Arm64Feature::SveF16f32mm:
		case trivial::cpu::Arm64Feature::SveB16mm:
		case trivial::cpu::Arm64Feature::SveBfscale:
		case trivial::cpu::Arm64Feature::SveAes2:
		case trivial::cpu::Arm64Feature::SvePerm256:
		case trivial::cpu::Arm64Feature::SveLut6:
		case trivial::cpu::Arm64Feature::Sme2p3:
		case trivial::cpu::Arm64Feature::SmeLut6:
		case trivial::cpu::Arm64Feature::SmeTmop:
		case trivial::cpu::Arm64Feature::SmeMop4:
		case trivial::cpu::Arm64Feature::SsveFexpa:
		case trivial::cpu::Arm64Feature::Mte:
		case trivial::cpu::Arm64Feature::Mte2:
		case trivial::cpu::Arm64Feature::Mte3:
		case trivial::cpu::Arm64Feature::MteAsync:
		case trivial::cpu::Arm64Feature::MteStoreOnly:
		case trivial::cpu::Arm64Feature::MteFar:
		case trivial::cpu::Arm64Feature::Poe:
		case trivial::cpu::Arm64Feature::Sb:
		case trivial::cpu::Arm64Feature::Ssbs:
		case trivial::cpu::Arm64Feature::Dit:
		case trivial::cpu::Arm64Feature::Bti:
		case trivial::cpu::Arm64Feature::PAuth:
		case trivial::cpu::Arm64Feature::Gcs:
		case trivial::cpu::Arm64Feature::Dpb:
		case trivial::cpu::Arm64Feature::Dpb2:
		case trivial::cpu::Arm64Feature::Ecv:
			return TRIVIAL_CPU_WINDOWS_ARM64_PF_NONE;
		case trivial::cpu::Arm64Feature::Count:
			break;
	}

	TRIVIAL_UNREACHABLE();
}

} // namespace

namespace trivial::cpu {

void probeIdentity() noexcept {
	FeatureSet<Arm64Feature> reported;

	for (std::size_t i = 0; i < static_cast<std::size_t>(Arm64Feature::Count); ++i) {
		const Arm64Feature kFeature = static_cast<Arm64Feature>(i);
		const DWORD kProcessorFeature = processorFeature(kFeature);

		if (kProcessorFeature == TRIVIAL_CPU_WINDOWS_ARM64_PF_NONE
		    || IsProcessorFeaturePresent(kProcessorFeature) == FALSE) {
			continue;
		}

		reported.insert(kFeature);

		if (featurePrerequisites(kFeature).contains(Arm64Feature::Fpmr)) {
			reported.insert(Arm64Feature::Fpmr);
		}
	}

	insertFeaturesWithPrerequisites<featurePrerequisites>(reported);

	HKEY processorKey = nullptr;

	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
	                  R"(HARDWARE\DESCRIPTION\System\CentralProcessor\0)",
	                  0,
	                  KEY_QUERY_VALUE,
	                  &processorKey)
	    != ERROR_SUCCESS) {
		TRIVIAL_LOG_ERROR_PREFIX("Cpu", "the OS did not report the processor's vendor and brand, so they are unknown");
		return;
	}

	std::uint64_t mainIdRegister = 0;
	DWORD mainIdSize = sizeof(mainIdRegister);

	if (RegGetValueA(processorKey, nullptr, "CP 4000", RRF_RT_REG_QWORD, nullptr, &mainIdRegister, &mainIdSize)
	    == ERROR_SUCCESS) {
		const char* kVendor = implementerName(decodeMainId(mainIdRegister).implementer);
		std::memcpy(detail::g_detectedCapabilities.identity.vendor.data(), kVendor, std::strlen(kVendor) + 1);
	}

	DWORD brandSize = 0;

	if (RegGetValueA(processorKey, nullptr, "ProcessorNameString", RRF_RT_REG_SZ, nullptr, nullptr, &brandSize)
	    == ERROR_SUCCESS) {
		std::vector<char> fullBrand(brandSize);

		if (RegGetValueA(processorKey,
		                 nullptr,
		                 "ProcessorNameString",
		                 RRF_RT_REG_SZ,
		                 nullptr,
		                 fullBrand.data(),
		                 &brandSize)
		    == ERROR_SUCCESS) {
			copyBrand({fullBrand.data(), brandSize});
		}
	}

	RegCloseKey(processorKey);
}

void probeMainIds(std::span<RawLogicalProcessor> logicalProcessors) noexcept {
	constexpr std::string_view kKeyPrefix = R"(HARDWARE\DESCRIPTION\System\CentralProcessor\)";
	static_assert(kKeyPrefix.size() + std::numeric_limits<std::uint32_t>::digits10 + 1
	                  < TRIVIAL_CPU_WINDOWS_ARM64_PROCESSOR_KEY_CAPACITY,
	              "The processor key should fit any processor number with its terminator");

	bool allReported = true;

	for (RawLogicalProcessor& logicalProcessor : logicalProcessors) {
		std::uint32_t registryNumber = 0;

		for (const RawLogicalProcessor& kOther : logicalProcessors) {
			registryNumber += kOther.processorNumber < logicalProcessor.processorNumber ? 1 : 0;
		}

		std::array<char, TRIVIAL_CPU_WINDOWS_ARM64_PROCESSOR_KEY_CAPACITY> keyName{};
		std::memcpy(keyName.data(), kKeyPrefix.data(), kKeyPrefix.size());
		(void)std::to_chars(&keyName[kKeyPrefix.size()], &keyName.back(), registryNumber);

		std::uint64_t mainIdRegister = 0;
		DWORD mainIdSize = sizeof(mainIdRegister);

		if (RegGetValueA(HKEY_LOCAL_MACHINE,
		                 keyName.data(),
		                 "CP 4000",
		                 RRF_RT_REG_QWORD,
		                 nullptr,
		                 &mainIdRegister,
		                 &mainIdSize)
		    == ERROR_SUCCESS) {
			logicalProcessor.mainId = decodeMainId(mainIdRegister);
		} else {
			logicalProcessor.mainId = {};
			allReported = false;
		}
	}

	if (!allReported) [[unlikely]] {
		TRIVIAL_LOG_ERROR_PREFIX("Cpu",
		                         "the OS did not report every logical processor's main ID, so those are unknown");
	}
}

} // namespace trivial::cpu

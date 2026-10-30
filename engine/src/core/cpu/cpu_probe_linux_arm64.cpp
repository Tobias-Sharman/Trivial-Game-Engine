#include "core/cpu/cpu_probe.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <sys/auxv.h>

#include <trivial/core/assert.h>
#include <trivial/core/cpu/cpu.h>
#include <trivial/core/cpu/cpu_feature_set.h>
#include <trivial/core/cpu/cpu_features_arm64.h>
#include <trivial/core/log.h>

#include "core/cpu/cpu_probe_arm64.h"

#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_AES (std::uint64_t{1} << 3U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_PMULL (std::uint64_t{1} << 4U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_SHA1 (std::uint64_t{1} << 5U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_SHA2 (std::uint64_t{1} << 6U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_CRC32 (std::uint64_t{1} << 7U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_ATOMICS (std::uint64_t{1} << 8U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_FPHP (std::uint64_t{1} << 9U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_ASIMDHP (std::uint64_t{1} << 10U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_CPUID (std::uint64_t{1} << 11U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_ASIMDRDM (std::uint64_t{1} << 12U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_JSCVT (std::uint64_t{1} << 13U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_FCMA (std::uint64_t{1} << 14U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_LRCPC (std::uint64_t{1} << 15U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_DCPOP (std::uint64_t{1} << 16U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_SHA3 (std::uint64_t{1} << 17U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_SM3 (std::uint64_t{1} << 18U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_SM4 (std::uint64_t{1} << 19U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_ASIMDDP (std::uint64_t{1} << 20U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_SHA512 (std::uint64_t{1} << 21U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_SVE (std::uint64_t{1} << 22U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_ASIMDFHM (std::uint64_t{1} << 23U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_DIT (std::uint64_t{1} << 24U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_USCAT (std::uint64_t{1} << 25U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_ILRCPC (std::uint64_t{1} << 26U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_FLAGM (std::uint64_t{1} << 27U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_SSBS (std::uint64_t{1} << 28U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_SB (std::uint64_t{1} << 29U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_PACA (std::uint64_t{1} << 30U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_PACG (std::uint64_t{1} << 31U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_GCS (std::uint64_t{1} << 32U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_CMPBR (std::uint64_t{1} << 33U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_FPRCVT (std::uint64_t{1} << 34U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_F8MM8 (std::uint64_t{1} << 35U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_F8MM4 (std::uint64_t{1} << 36U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_SVE_F16MM (std::uint64_t{1} << 37U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_SVE_ELTPERM (std::uint64_t{1} << 38U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_SVE_AES2 (std::uint64_t{1} << 39U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_SVE_BFSCALE (std::uint64_t{1} << 40U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_SVE2P2 (std::uint64_t{1} << 41U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_SME2P2 (std::uint64_t{1} << 42U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_SME_SBITPERM (std::uint64_t{1} << 43U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_SME_AES (std::uint64_t{1} << 44U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_SME_SFEXPA (std::uint64_t{1} << 45U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_SME_STMOP (std::uint64_t{1} << 46U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP_SME_SMOP4 (std::uint64_t{1} << 47U)

#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_DCPODP (std::uint64_t{1} << 0U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SVE2 (std::uint64_t{1} << 1U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SVEAES (std::uint64_t{1} << 2U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SVEPMULL (std::uint64_t{1} << 3U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SVEBITPERM (std::uint64_t{1} << 4U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SVESHA3 (std::uint64_t{1} << 5U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SVESM4 (std::uint64_t{1} << 6U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_FLAGM2 (std::uint64_t{1} << 7U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_FRINT (std::uint64_t{1} << 8U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SVEI8MM (std::uint64_t{1} << 9U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SVEF32MM (std::uint64_t{1} << 10U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SVEF64MM (std::uint64_t{1} << 11U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SVEBF16 (std::uint64_t{1} << 12U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_I8MM (std::uint64_t{1} << 13U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_BF16 (std::uint64_t{1} << 14U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_RNG (std::uint64_t{1} << 16U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_BTI (std::uint64_t{1} << 17U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_MTE (std::uint64_t{1} << 18U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_ECV (std::uint64_t{1} << 19U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_AFP (std::uint64_t{1} << 20U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_RPRES (std::uint64_t{1} << 21U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_MTE3 (std::uint64_t{1} << 22U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SME (std::uint64_t{1} << 23U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SME_I16I64 (std::uint64_t{1} << 24U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SME_F64F64 (std::uint64_t{1} << 25U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SME_FA64 (std::uint64_t{1} << 30U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_WFXT (std::uint64_t{1} << 31U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_EBF16 (std::uint64_t{1} << 32U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SVE_EBF16 (std::uint64_t{1} << 33U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_CSSC (std::uint64_t{1} << 34U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_RPRFM (std::uint64_t{1} << 35U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SVE2P1 (std::uint64_t{1} << 36U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SME2 (std::uint64_t{1} << 37U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SME2P1 (std::uint64_t{1} << 38U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SME_B16B16 (std::uint64_t{1} << 41U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SME_F16F16 (std::uint64_t{1} << 42U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_MOPS (std::uint64_t{1} << 43U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_HBC (std::uint64_t{1} << 44U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SVE_B16B16 (std::uint64_t{1} << 45U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_LRCPC3 (std::uint64_t{1} << 46U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_LSE128 (std::uint64_t{1} << 47U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_FPMR (std::uint64_t{1} << 48U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_LUT (std::uint64_t{1} << 49U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_FAMINMAX (std::uint64_t{1} << 50U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_F8CVT (std::uint64_t{1} << 51U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_F8FMA (std::uint64_t{1} << 52U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_F8DP4 (std::uint64_t{1} << 53U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_F8DP2 (std::uint64_t{1} << 54U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SME_LUTV2 (std::uint64_t{1} << 57U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SME_F8F16 (std::uint64_t{1} << 58U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SME_F8F32 (std::uint64_t{1} << 59U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SME_SF8FMA (std::uint64_t{1} << 60U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SME_SF8DP4 (std::uint64_t{1} << 61U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SME_SF8DP2 (std::uint64_t{1} << 62U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP2_POE (std::uint64_t{1} << 63U)

#define TRIVIAL_CPU_LINUX_ARM64_HWCAP3_MTE_FAR (std::uint64_t{1} << 0U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP3_MTE_STORE_ONLY (std::uint64_t{1} << 1U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP3_LSFE (std::uint64_t{1} << 2U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP3_SVE_B16MM (std::uint64_t{1} << 4U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP3_SVE2P3 (std::uint64_t{1} << 5U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP3_SME_LUT6 (std::uint64_t{1} << 6U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP3_SME2P3 (std::uint64_t{1} << 7U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP3_F16MM (std::uint64_t{1} << 8U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP3_F16F32DOT (std::uint64_t{1} << 9U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP3_F16F32MM (std::uint64_t{1} << 10U)
#define TRIVIAL_CPU_LINUX_ARM64_HWCAP3_SVE_LUT6 (std::uint64_t{1} << 11U)
#define TRIVIAL_CPU_LINUX_ARM64_AT_HWCAP3 29UL

namespace {

struct HwcapSource {
	unsigned long type;
	std::uint64_t bits;
};

[[nodiscard]] constexpr HwcapSource hwcapSource(trivial::cpu::Arm64Feature feature) noexcept {
	switch (feature) {
		case trivial::cpu::Arm64Feature::Lse:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_ATOMICS};

		case trivial::cpu::Arm64Feature::Lse2:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_USCAT};

		case trivial::cpu::Arm64Feature::Lse128:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_LSE128};

		case trivial::cpu::Arm64Feature::Lsfe:
			return {.type = TRIVIAL_CPU_LINUX_ARM64_AT_HWCAP3, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP3_LSFE};

		case trivial::cpu::Arm64Feature::Lrcpc:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_LRCPC};

		case trivial::cpu::Arm64Feature::Lrcpc2:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_ILRCPC};

		case trivial::cpu::Arm64Feature::Lrcpc3:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_LRCPC3};

		case trivial::cpu::Arm64Feature::Mops:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_MOPS};

		case trivial::cpu::Arm64Feature::Rprfm:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_RPRFM};

		case trivial::cpu::Arm64Feature::Wfxt:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_WFXT};

		case trivial::cpu::Arm64Feature::Crc32:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_CRC32};

		case trivial::cpu::Arm64Feature::Aes:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_AES};

		case trivial::cpu::Arm64Feature::Pmull:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_PMULL};

		case trivial::cpu::Arm64Feature::Sha1:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_SHA1};

		case trivial::cpu::Arm64Feature::Sha256:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_SHA2};

		case trivial::cpu::Arm64Feature::Sha3:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_SHA3};

		case trivial::cpu::Arm64Feature::Sha512:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_SHA512};

		case trivial::cpu::Arm64Feature::Sm3:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_SM3};

		case trivial::cpu::Arm64Feature::Sm4:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_SM4};

		case trivial::cpu::Arm64Feature::Rng:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_RNG};

		case trivial::cpu::Arm64Feature::Cssc:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_CSSC};

		case trivial::cpu::Arm64Feature::FlagM:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_FLAGM};

		case trivial::cpu::Arm64Feature::FlagM2:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_FLAGM2};

		case trivial::cpu::Arm64Feature::Jscvt:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_JSCVT};

		case trivial::cpu::Arm64Feature::Hbc:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_HBC};

		case trivial::cpu::Arm64Feature::Cmpbr:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_CMPBR};

		case trivial::cpu::Arm64Feature::Frintts:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_FRINT};

		case trivial::cpu::Arm64Feature::Fprcvt:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_FPRCVT};

		case trivial::cpu::Arm64Feature::Afp:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_AFP};

		case trivial::cpu::Arm64Feature::Rpres:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_RPRES};

		case trivial::cpu::Arm64Feature::Rdm:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_ASIMDRDM};

		case trivial::cpu::Arm64Feature::DotProd:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_ASIMDDP};

		case trivial::cpu::Arm64Feature::Fp16:
			return {
			    .type = AT_HWCAP,
			    .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_FPHP | TRIVIAL_CPU_LINUX_ARM64_HWCAP_ASIMDHP,
			};

		case trivial::cpu::Arm64Feature::Fhm:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_ASIMDFHM};

		case trivial::cpu::Arm64Feature::F16f32dot:
			return {.type = TRIVIAL_CPU_LINUX_ARM64_AT_HWCAP3, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP3_F16F32DOT};

		case trivial::cpu::Arm64Feature::F16f32mm:
			return {.type = TRIVIAL_CPU_LINUX_ARM64_AT_HWCAP3, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP3_F16F32MM};

		case trivial::cpu::Arm64Feature::F16mm:
			return {.type = TRIVIAL_CPU_LINUX_ARM64_AT_HWCAP3, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP3_F16MM};

		case trivial::cpu::Arm64Feature::Fcma:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_FCMA};

		case trivial::cpu::Arm64Feature::I8mm:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_I8MM};

		case trivial::cpu::Arm64Feature::Bf16:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_BF16};

		case trivial::cpu::Arm64Feature::Ebf16:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_EBF16};

		case trivial::cpu::Arm64Feature::Lut:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_LUT};

		case trivial::cpu::Arm64Feature::Faminmax:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_FAMINMAX};

		case trivial::cpu::Arm64Feature::Fpmr:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_FPMR};

		case trivial::cpu::Arm64Feature::Fp8:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_F8CVT};

		case trivial::cpu::Arm64Feature::Fp8Fma:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_F8FMA};

		case trivial::cpu::Arm64Feature::Fp8Dot4:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_F8DP4};

		case trivial::cpu::Arm64Feature::Fp8Dot2:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_F8DP2};

		case trivial::cpu::Arm64Feature::F8f16mm:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_F8MM4};

		case trivial::cpu::Arm64Feature::F8f32mm:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_F8MM8};

		case trivial::cpu::Arm64Feature::Sve:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_SVE};

		case trivial::cpu::Arm64Feature::Sve2:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SVE2};

		case trivial::cpu::Arm64Feature::Sve2p1:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SVE2P1};

		case trivial::cpu::Arm64Feature::Sve2p2:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_SVE2P2};

		case trivial::cpu::Arm64Feature::Sve2p3:
			return {.type = TRIVIAL_CPU_LINUX_ARM64_AT_HWCAP3, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP3_SVE2P3};

		case trivial::cpu::Arm64Feature::SveI8mm:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SVEI8MM};

		case trivial::cpu::Arm64Feature::SveBf16:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SVEBF16};

		case trivial::cpu::Arm64Feature::SveEbf16:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SVE_EBF16};

		case trivial::cpu::Arm64Feature::SveF32mm:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SVEF32MM};

		case trivial::cpu::Arm64Feature::SveF64mm:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SVEF64MM};

		case trivial::cpu::Arm64Feature::SveF16f32mm:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_SVE_F16MM};

		case trivial::cpu::Arm64Feature::SveB16b16:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SVE_B16B16};

		case trivial::cpu::Arm64Feature::SveB16mm:
			return {.type = TRIVIAL_CPU_LINUX_ARM64_AT_HWCAP3, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP3_SVE_B16MM};

		case trivial::cpu::Arm64Feature::SveBfscale:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_SVE_BFSCALE};

		case trivial::cpu::Arm64Feature::SveAes:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SVEAES};

		case trivial::cpu::Arm64Feature::SvePmull128:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SVEPMULL};

		case trivial::cpu::Arm64Feature::SveAes2:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_SVE_AES2};

		case trivial::cpu::Arm64Feature::SveBitperm:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SVEBITPERM};

		case trivial::cpu::Arm64Feature::SveSha3:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SVESHA3};

		case trivial::cpu::Arm64Feature::SveSm4:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SVESM4};

		case trivial::cpu::Arm64Feature::SvePerm256:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_SVE_ELTPERM};

		case trivial::cpu::Arm64Feature::SveLut6:
			return {.type = TRIVIAL_CPU_LINUX_ARM64_AT_HWCAP3, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP3_SVE_LUT6};

		case trivial::cpu::Arm64Feature::Sme:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SME};

		case trivial::cpu::Arm64Feature::Sme2:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SME2};

		case trivial::cpu::Arm64Feature::Sme2p1:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SME2P1};

		case trivial::cpu::Arm64Feature::Sme2p2:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_SME2P2};

		case trivial::cpu::Arm64Feature::Sme2p3:
			return {.type = TRIVIAL_CPU_LINUX_ARM64_AT_HWCAP3, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP3_SME2P3};

		case trivial::cpu::Arm64Feature::SmeF64F64:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SME_F64F64};

		case trivial::cpu::Arm64Feature::SmeI16I64:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SME_I16I64};

		case trivial::cpu::Arm64Feature::SmeF16F16:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SME_F16F16};

		case trivial::cpu::Arm64Feature::SmeB16B16:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SME_B16B16};

		case trivial::cpu::Arm64Feature::SmeF8F16:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SME_F8F16};

		case trivial::cpu::Arm64Feature::SmeF8F32:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SME_F8F32};

		case trivial::cpu::Arm64Feature::SmeLutv2:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SME_LUTV2};

		case trivial::cpu::Arm64Feature::SmeLut6:
			return {.type = TRIVIAL_CPU_LINUX_ARM64_AT_HWCAP3, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP3_SME_LUT6};

		case trivial::cpu::Arm64Feature::SmeTmop:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_SME_STMOP};

		case trivial::cpu::Arm64Feature::SmeMop4:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_SME_SMOP4};

		case trivial::cpu::Arm64Feature::SmeFa64:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SME_FA64};

		case trivial::cpu::Arm64Feature::SsveAes:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_SME_AES};

		case trivial::cpu::Arm64Feature::SsveBitperm:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_SME_SBITPERM};

		case trivial::cpu::Arm64Feature::SsveFexpa:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_SME_SFEXPA};

		case trivial::cpu::Arm64Feature::SsveFp8Fma:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SME_SF8FMA};

		case trivial::cpu::Arm64Feature::SsveFp8Dot4:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SME_SF8DP4};

		case trivial::cpu::Arm64Feature::SsveFp8Dot2:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_SME_SF8DP2};

		case trivial::cpu::Arm64Feature::Mte:
		case trivial::cpu::Arm64Feature::Mte2:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_MTE};

		case trivial::cpu::Arm64Feature::Mte3:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_MTE3};

		case trivial::cpu::Arm64Feature::MteStoreOnly:
			return {.type = TRIVIAL_CPU_LINUX_ARM64_AT_HWCAP3, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP3_MTE_STORE_ONLY};

		case trivial::cpu::Arm64Feature::MteFar:
			return {.type = TRIVIAL_CPU_LINUX_ARM64_AT_HWCAP3, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP3_MTE_FAR};

		case trivial::cpu::Arm64Feature::Poe:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_POE};

		case trivial::cpu::Arm64Feature::Sb:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_SB};

		case trivial::cpu::Arm64Feature::Ssbs:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_SSBS};

		case trivial::cpu::Arm64Feature::Dit:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_DIT};

		case trivial::cpu::Arm64Feature::Bti:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_BTI};

		case trivial::cpu::Arm64Feature::PAuth:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_PACA | TRIVIAL_CPU_LINUX_ARM64_HWCAP_PACG};

		case trivial::cpu::Arm64Feature::Gcs:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_GCS};

		case trivial::cpu::Arm64Feature::Dpb:
			return {.type = AT_HWCAP, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP_DCPOP};

		case trivial::cpu::Arm64Feature::Dpb2:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_DCPODP};

		case trivial::cpu::Arm64Feature::Ecv:
			return {.type = AT_HWCAP2, .bits = TRIVIAL_CPU_LINUX_ARM64_HWCAP2_ECV};

		case trivial::cpu::Arm64Feature::FpSyncExceptions:
		case trivial::cpu::Arm64Feature::SsveF8f16mm:
		case trivial::cpu::Arm64Feature::SsveF8f32mm:
		case trivial::cpu::Arm64Feature::MteAsync:
			return {.type = AT_NULL, .bits = 0};
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
		const HwcapSource kSource = hwcapSource(kFeature);

		if (kSource.type != AT_NULL && (getauxval(kSource.type) & kSource.bits) == kSource.bits) {
			reported.insert(kFeature);
		}
	}

	insertFeaturesWithPrerequisites<featurePrerequisites>(reported);

	if ((getauxval(AT_HWCAP) & TRIVIAL_CPU_LINUX_ARM64_HWCAP_CPUID) != 0) {
		std::uint64_t mainIdRegister = 0;
		__asm__ volatile("mrs %0, midr_el1" : "=r"(mainIdRegister)); // NOLINT(portability-no-assembler)

		const char* kVendor = implementerName(decodeMainId(mainIdRegister).implementer);
		std::memcpy(detail::g_detectedCapabilities.identity.vendor.data(), kVendor, std::strlen(kVendor) + 1);
	} else {
		TRIVIAL_LOG_ERROR_PREFIX("Cpu", "the OS does not allow reading the main ID, so the vendor is unknown");
	}
}

void probeMainIds(std::span<RawLogicalProcessor> logicalProcessors) noexcept {
	TRIVIAL_LOG_INFO_PREFIX("Cpu",
	                        "main IDs are not read on Linux yet, as they need sysfs file reads, so they are unknown");

	for (RawLogicalProcessor& logicalProcessor : logicalProcessors) {
		logicalProcessor.mainId = {};
	}
}

} // namespace trivial::cpu

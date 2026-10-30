#include "core/cpu/cpu_probe.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>

#include <trivial/core/assert.h>
#include <trivial/core/compiler.h>
#include <trivial/core/cpu/cpu.h>
#include <trivial/core/cpu/cpu_feature_set.h>
#include <trivial/core/cpu/cpu_features_x86_64.h>
#include <trivial/core/platform.h>

#if TRIVIAL_COMPILER_MSVC || TRIVIAL_COMPILER_CLANG_CL
#include <bit>
#include <intrin.h>
#else
#include <cpuid.h>
#endif // Compiler check

#define TRIVIAL_CPU_X86_64_LEAF_VENDOR std::uint32_t{0x0}
#define TRIVIAL_CPU_X86_64_LEAF_FEATURES std::uint32_t{0x1}
#define TRIVIAL_CPU_X86_64_LEAF_STRUCTURED_FEATURES std::uint32_t{0x7}
#define TRIVIAL_CPU_X86_64_LEAF_EXTENDED_STATE std::uint32_t{0xD}
#define TRIVIAL_CPU_X86_64_LEAF_AVX10 std::uint32_t{0x24}
#define TRIVIAL_CPU_X86_64_LEAF_EXTENDED_MAX std::uint32_t{0x80000000}
#define TRIVIAL_CPU_X86_64_LEAF_EXTENDED_FEATURES std::uint32_t{0x80000001}
#define TRIVIAL_CPU_X86_64_LEAF_BRAND_FIRST std::uint32_t{0x80000002}
#define TRIVIAL_CPU_X86_64_LEAF_BRAND_LAST std::uint32_t{0x80000004}
#define TRIVIAL_CPU_X86_64_LEAF_POWER_MANAGEMENT std::uint32_t{0x80000007}
#define TRIVIAL_CPU_X86_64_LEAF_EXTENDED_CAPACITY std::uint32_t{0x80000008}
#define TRIVIAL_CPU_X86_64_LEAF_EXTENDED_FEATURES_2 std::uint32_t{0x80000021}

#define TRIVIAL_CPU_X86_64_AVX10_BIT 19U

#define TRIVIAL_CPU_X86_64_CMPXCHG16B_BIT 13U
#define TRIVIAL_CPU_X86_64_CMPCCXADD_BIT 7U
#define TRIVIAL_CPU_X86_64_RAO_INT_BIT 3U
#define TRIVIAL_CPU_X86_64_RTM_BIT 11U
#define TRIVIAL_CPU_X86_64_MOVDIRI_BIT 27U
#define TRIVIAL_CPU_X86_64_MOVDIR64B_BIT 28U
#define TRIVIAL_CPU_X86_64_SERIALIZE_BIT 14U
#define TRIVIAL_CPU_X86_64_ENHANCED_REP_MOVSB_BIT 9U
#define TRIVIAL_CPU_X86_64_FAST_SHORT_REP_MOVSB_BIT 4U
#define TRIVIAL_CPU_X86_64_FAST_ZERO_LENGTH_REP_MOVSB_BIT 10U
#define TRIVIAL_CPU_X86_64_FAST_SHORT_REP_STOSB_BIT 11U
#define TRIVIAL_CPU_X86_64_FAST_SHORT_REP_CMPSB_SCASB_BIT 12U
#define TRIVIAL_CPU_X86_64_WAITPKG_BIT 5U
#define TRIVIAL_CPU_X86_64_MONITORX_BIT 29U
#define TRIVIAL_CPU_X86_64_CLFLUSHOPT_BIT 23U
#define TRIVIAL_CPU_X86_64_CLWB_BIT 24U
#define TRIVIAL_CPU_X86_64_CLDEMOTE_BIT 25U
#define TRIVIAL_CPU_X86_64_CLZERO_BIT 0U
#define TRIVIAL_CPU_X86_64_PREFETCHW_BIT 8U
#define TRIVIAL_CPU_X86_64_PREFETCHI_BIT 14U
#define TRIVIAL_CPU_X86_64_MOVRS_BIT 31U
#define TRIVIAL_CPU_X86_64_PCLMULQDQ_BIT 1U
#define TRIVIAL_CPU_X86_64_AESNI_BIT 25U
#define TRIVIAL_CPU_X86_64_SHA_BIT 29U
#define TRIVIAL_CPU_X86_64_SHA512_BIT 0U
#define TRIVIAL_CPU_X86_64_SM3_BIT 1U
#define TRIVIAL_CPU_X86_64_SM4_BIT 2U
#define TRIVIAL_CPU_X86_64_GFNI_BIT 8U
#define TRIVIAL_CPU_X86_64_VAES_BIT 9U
#define TRIVIAL_CPU_X86_64_VPCLMULQDQ_BIT 10U
#define TRIVIAL_CPU_X86_64_RDRAND_BIT 30U
#define TRIVIAL_CPU_X86_64_RDSEED_BIT 18U
#define TRIVIAL_CPU_X86_64_POPCNT_BIT 23U
#define TRIVIAL_CPU_X86_64_LZCNT_BIT 5U
#define TRIVIAL_CPU_X86_64_BMI1_BIT 3U
#define TRIVIAL_CPU_X86_64_BMI2_BIT 8U
#define TRIVIAL_CPU_X86_64_TBM_BIT 21U
#define TRIVIAL_CPU_X86_64_ADX_BIT 19U
#define TRIVIAL_CPU_X86_64_MOVBE_BIT 22U
#define TRIVIAL_CPU_X86_64_LAHF_SAHF_BIT 0U
#define TRIVIAL_CPU_X86_64_RDPID_BIT 22U
#define TRIVIAL_CPU_X86_64_SSE3_BIT 0U
#define TRIVIAL_CPU_X86_64_SSSE3_BIT 9U
#define TRIVIAL_CPU_X86_64_SSE41_BIT 19U
#define TRIVIAL_CPU_X86_64_SSE42_BIT 20U
#define TRIVIAL_CPU_X86_64_SSE4A_BIT 6U
#define TRIVIAL_CPU_X86_64_AVX_BIT 28U
#define TRIVIAL_CPU_X86_64_AVX2_BIT 5U
#define TRIVIAL_CPU_X86_64_FMA_BIT 12U
#define TRIVIAL_CPU_X86_64_F16C_BIT 29U
#define TRIVIAL_CPU_X86_64_AVX_VNNI_BIT 4U
#define TRIVIAL_CPU_X86_64_AVX_VNNI_INT8_BIT 4U
#define TRIVIAL_CPU_X86_64_AVX_VNNI_INT16_BIT 10U
#define TRIVIAL_CPU_X86_64_AVX_IFMA_BIT 23U
#define TRIVIAL_CPU_X86_64_AVX_NE_CONVERT_BIT 5U
#define TRIVIAL_CPU_X86_64_XOP_BIT 11U
#define TRIVIAL_CPU_X86_64_FMA4_BIT 16U
#define TRIVIAL_CPU_X86_64_AVX512_F_BIT 16U
#define TRIVIAL_CPU_X86_64_AVX512_DQ_BIT 17U
#define TRIVIAL_CPU_X86_64_AVX512_CD_BIT 28U
#define TRIVIAL_CPU_X86_64_AVX512_BW_BIT 30U
#define TRIVIAL_CPU_X86_64_AVX512_VL_BIT 31U
#define TRIVIAL_CPU_X86_64_AVX512_IFMA_BIT 21U
#define TRIVIAL_CPU_X86_64_AVX512_VBMI_BIT 1U
#define TRIVIAL_CPU_X86_64_AVX512_VBMI2_BIT 6U
#define TRIVIAL_CPU_X86_64_AVX512_VNNI_BIT 11U
#define TRIVIAL_CPU_X86_64_AVX512_BITALG_BIT 12U
#define TRIVIAL_CPU_X86_64_AVX512_VPOPCNTDQ_BIT 14U
#define TRIVIAL_CPU_X86_64_AVX512_VP2INTERSECT_BIT 8U
#define TRIVIAL_CPU_X86_64_AVX512_FP16_BIT 23U
#define TRIVIAL_CPU_X86_64_AVX512_BF16_BIT 5U
#define TRIVIAL_CPU_X86_64_AVX10_V1_BIT 0U
#define TRIVIAL_CPU_X86_64_AVX10_V2_BIT 1U
#define TRIVIAL_CPU_X86_64_APX_F_BIT 21U
#define TRIVIAL_CPU_X86_64_XSAVE_BIT 26U
#define TRIVIAL_CPU_X86_64_OSXSAVE_BIT 27U
#define TRIVIAL_CPU_X86_64_XSAVEOPT_BIT 0U
#define TRIVIAL_CPU_X86_64_XSAVEC_BIT 1U
#define TRIVIAL_CPU_X86_64_PKU_BIT 4U
#define TRIVIAL_CPU_X86_64_RDTSCP_BIT 27U
#define TRIVIAL_CPU_X86_64_INVARIANT_TSC_BIT 8U
#define TRIVIAL_CPU_X86_64_HYPERVISOR_PRESENT_BIT 31U
#define TRIVIAL_CPU_X86_64_HYBRID_BIT 15U
#define TRIVIAL_CPU_X86_64_AMD_PREFETCHI_BIT 20U

#define TRIVIAL_CPU_X86_64_XCR0_AVX_STATE std::uint64_t{0x6}
#define TRIVIAL_CPU_X86_64_XCR0_AVX512_STATE std::uint64_t{0xE0}
#define TRIVIAL_CPU_X86_64_XCR0_APX_STATE std::uint64_t{0x80000}
#define TRIVIAL_CPU_X86_64_XCR0_HIGH_SHIFT 32U

#define TRIVIAL_CPU_X86_64_SIGNATURE_STEPPING_HIGH_BIT 3U
#define TRIVIAL_CPU_X86_64_SIGNATURE_STEPPING_LOW_BIT 0U
#define TRIVIAL_CPU_X86_64_SIGNATURE_MODEL_HIGH_BIT 7U
#define TRIVIAL_CPU_X86_64_SIGNATURE_MODEL_LOW_BIT 4U
#define TRIVIAL_CPU_X86_64_SIGNATURE_FAMILY_HIGH_BIT 11U
#define TRIVIAL_CPU_X86_64_SIGNATURE_FAMILY_LOW_BIT 8U
#define TRIVIAL_CPU_X86_64_SIGNATURE_EXTENDED_MODEL_HIGH_BIT 19U
#define TRIVIAL_CPU_X86_64_SIGNATURE_EXTENDED_MODEL_LOW_BIT 16U
#define TRIVIAL_CPU_X86_64_SIGNATURE_EXTENDED_FAMILY_HIGH_BIT 27U
#define TRIVIAL_CPU_X86_64_SIGNATURE_EXTENDED_FAMILY_LOW_BIT 20U

#define TRIVIAL_CPU_X86_64_FAMILY_06H std::uint32_t{0x6}
#define TRIVIAL_CPU_X86_64_FAMILY_0FH std::uint32_t{0xF}

namespace {

enum class CpuidRegister : std::uint8_t {
	Leaf1Eax,
	Leaf1Ecx,
	Leaf7Ebx,
	Leaf7Ecx,
	Leaf7Edx,
	Leaf7Subleaf1Eax,
	Leaf7Subleaf1Edx,
	Extended1Ecx,
	Extended1Edx,
	Extended7Edx,
	Extended8Ebx,
	Extended21Eax,
	Leaf0DSubleaf1Eax,
	Leaf24Ebx,
	Avx10Versions,

	Count,
};

enum class RegisterState : std::uint8_t {
	None,
	Xsave,
	Ymm,
	Zmm,
	Apx,

	Count,
};

struct CpuidBit {
	CpuidRegister cpuidRegister;
	std::uint32_t bit;
};

[[nodiscard]] constexpr CpuidBit cpuidBit(trivial::cpu::X86_64Feature feature) noexcept {
	switch (feature) {
		case trivial::cpu::X86_64Feature::Cmpxchg16b:
			return {.cpuidRegister = CpuidRegister::Leaf1Ecx, .bit = TRIVIAL_CPU_X86_64_CMPXCHG16B_BIT};

		case trivial::cpu::X86_64Feature::Cmpccxadd:
			return {.cpuidRegister = CpuidRegister::Leaf7Subleaf1Eax, .bit = TRIVIAL_CPU_X86_64_CMPCCXADD_BIT};

		case trivial::cpu::X86_64Feature::RaoInt:
			return {.cpuidRegister = CpuidRegister::Leaf7Subleaf1Eax, .bit = TRIVIAL_CPU_X86_64_RAO_INT_BIT};

		case trivial::cpu::X86_64Feature::Rtm:
			return {.cpuidRegister = CpuidRegister::Leaf7Ebx, .bit = TRIVIAL_CPU_X86_64_RTM_BIT};

		case trivial::cpu::X86_64Feature::Movdiri:
			return {.cpuidRegister = CpuidRegister::Leaf7Ecx, .bit = TRIVIAL_CPU_X86_64_MOVDIRI_BIT};

		case trivial::cpu::X86_64Feature::Movdir64b:
			return {.cpuidRegister = CpuidRegister::Leaf7Ecx, .bit = TRIVIAL_CPU_X86_64_MOVDIR64B_BIT};

		case trivial::cpu::X86_64Feature::Serialize:
			return {.cpuidRegister = CpuidRegister::Leaf7Edx, .bit = TRIVIAL_CPU_X86_64_SERIALIZE_BIT};

		case trivial::cpu::X86_64Feature::EnhancedRepMovsb:
			return {.cpuidRegister = CpuidRegister::Leaf7Ebx, .bit = TRIVIAL_CPU_X86_64_ENHANCED_REP_MOVSB_BIT};

		case trivial::cpu::X86_64Feature::FastShortRepMovsb:
			return {.cpuidRegister = CpuidRegister::Leaf7Edx, .bit = TRIVIAL_CPU_X86_64_FAST_SHORT_REP_MOVSB_BIT};

		case trivial::cpu::X86_64Feature::FastZeroLengthRepMovsb:
			return {
			    .cpuidRegister = CpuidRegister::Leaf7Subleaf1Eax,
			    .bit = TRIVIAL_CPU_X86_64_FAST_ZERO_LENGTH_REP_MOVSB_BIT,
			};

		case trivial::cpu::X86_64Feature::FastShortRepStosb:
			return {
			    .cpuidRegister = CpuidRegister::Leaf7Subleaf1Eax,
			    .bit = TRIVIAL_CPU_X86_64_FAST_SHORT_REP_STOSB_BIT,
			};

		case trivial::cpu::X86_64Feature::FastShortRepCmpsbScasb:
			return {
			    .cpuidRegister = CpuidRegister::Leaf7Subleaf1Eax,
			    .bit = TRIVIAL_CPU_X86_64_FAST_SHORT_REP_CMPSB_SCASB_BIT,
			};

		case trivial::cpu::X86_64Feature::Waitpkg:
			return {.cpuidRegister = CpuidRegister::Leaf7Ecx, .bit = TRIVIAL_CPU_X86_64_WAITPKG_BIT};

		case trivial::cpu::X86_64Feature::Monitorx:
			return {.cpuidRegister = CpuidRegister::Extended1Ecx, .bit = TRIVIAL_CPU_X86_64_MONITORX_BIT};

		case trivial::cpu::X86_64Feature::Clflushopt:
			return {.cpuidRegister = CpuidRegister::Leaf7Ebx, .bit = TRIVIAL_CPU_X86_64_CLFLUSHOPT_BIT};

		case trivial::cpu::X86_64Feature::Clwb:
			return {.cpuidRegister = CpuidRegister::Leaf7Ebx, .bit = TRIVIAL_CPU_X86_64_CLWB_BIT};

		case trivial::cpu::X86_64Feature::Cldemote:
			return {.cpuidRegister = CpuidRegister::Leaf7Ecx, .bit = TRIVIAL_CPU_X86_64_CLDEMOTE_BIT};

		case trivial::cpu::X86_64Feature::Clzero:
			return {.cpuidRegister = CpuidRegister::Extended8Ebx, .bit = TRIVIAL_CPU_X86_64_CLZERO_BIT};

		case trivial::cpu::X86_64Feature::Prefetchw:
			return {.cpuidRegister = CpuidRegister::Extended1Ecx, .bit = TRIVIAL_CPU_X86_64_PREFETCHW_BIT};

		case trivial::cpu::X86_64Feature::Prefetchi:
			return {.cpuidRegister = CpuidRegister::Leaf7Subleaf1Edx, .bit = TRIVIAL_CPU_X86_64_PREFETCHI_BIT};

		case trivial::cpu::X86_64Feature::Movrs:
			return {.cpuidRegister = CpuidRegister::Leaf7Subleaf1Eax, .bit = TRIVIAL_CPU_X86_64_MOVRS_BIT};

		case trivial::cpu::X86_64Feature::Pclmulqdq:
			return {.cpuidRegister = CpuidRegister::Leaf1Ecx, .bit = TRIVIAL_CPU_X86_64_PCLMULQDQ_BIT};

		case trivial::cpu::X86_64Feature::Aesni:
			return {.cpuidRegister = CpuidRegister::Leaf1Ecx, .bit = TRIVIAL_CPU_X86_64_AESNI_BIT};

		case trivial::cpu::X86_64Feature::Sha:
			return {.cpuidRegister = CpuidRegister::Leaf7Ebx, .bit = TRIVIAL_CPU_X86_64_SHA_BIT};

		case trivial::cpu::X86_64Feature::Sha512:
			return {.cpuidRegister = CpuidRegister::Leaf7Subleaf1Eax, .bit = TRIVIAL_CPU_X86_64_SHA512_BIT};

		case trivial::cpu::X86_64Feature::Sm3:
			return {.cpuidRegister = CpuidRegister::Leaf7Subleaf1Eax, .bit = TRIVIAL_CPU_X86_64_SM3_BIT};

		case trivial::cpu::X86_64Feature::Sm4:
			return {.cpuidRegister = CpuidRegister::Leaf7Subleaf1Eax, .bit = TRIVIAL_CPU_X86_64_SM4_BIT};

		case trivial::cpu::X86_64Feature::Gfni:
			return {.cpuidRegister = CpuidRegister::Leaf7Ecx, .bit = TRIVIAL_CPU_X86_64_GFNI_BIT};

		case trivial::cpu::X86_64Feature::Vaes:
			return {.cpuidRegister = CpuidRegister::Leaf7Ecx, .bit = TRIVIAL_CPU_X86_64_VAES_BIT};

		case trivial::cpu::X86_64Feature::Vpclmulqdq:
			return {.cpuidRegister = CpuidRegister::Leaf7Ecx, .bit = TRIVIAL_CPU_X86_64_VPCLMULQDQ_BIT};

		case trivial::cpu::X86_64Feature::Rdrand:
			return {.cpuidRegister = CpuidRegister::Leaf1Ecx, .bit = TRIVIAL_CPU_X86_64_RDRAND_BIT};

		case trivial::cpu::X86_64Feature::Rdseed:
			return {.cpuidRegister = CpuidRegister::Leaf7Ebx, .bit = TRIVIAL_CPU_X86_64_RDSEED_BIT};

		case trivial::cpu::X86_64Feature::Popcnt:
			return {.cpuidRegister = CpuidRegister::Leaf1Ecx, .bit = TRIVIAL_CPU_X86_64_POPCNT_BIT};

		case trivial::cpu::X86_64Feature::Lzcnt:
			return {.cpuidRegister = CpuidRegister::Extended1Ecx, .bit = TRIVIAL_CPU_X86_64_LZCNT_BIT};

		case trivial::cpu::X86_64Feature::Bmi1:
			return {.cpuidRegister = CpuidRegister::Leaf7Ebx, .bit = TRIVIAL_CPU_X86_64_BMI1_BIT};

		case trivial::cpu::X86_64Feature::Bmi2:
			return {.cpuidRegister = CpuidRegister::Leaf7Ebx, .bit = TRIVIAL_CPU_X86_64_BMI2_BIT};

		case trivial::cpu::X86_64Feature::Tbm:
			return {.cpuidRegister = CpuidRegister::Extended1Ecx, .bit = TRIVIAL_CPU_X86_64_TBM_BIT};

		case trivial::cpu::X86_64Feature::Adx:
			return {.cpuidRegister = CpuidRegister::Leaf7Ebx, .bit = TRIVIAL_CPU_X86_64_ADX_BIT};

		case trivial::cpu::X86_64Feature::Movbe:
			return {.cpuidRegister = CpuidRegister::Leaf1Ecx, .bit = TRIVIAL_CPU_X86_64_MOVBE_BIT};

		case trivial::cpu::X86_64Feature::LahfSahf:
			return {.cpuidRegister = CpuidRegister::Extended1Ecx, .bit = TRIVIAL_CPU_X86_64_LAHF_SAHF_BIT};

		case trivial::cpu::X86_64Feature::Rdpid:
			return {.cpuidRegister = CpuidRegister::Leaf7Ecx, .bit = TRIVIAL_CPU_X86_64_RDPID_BIT};

		case trivial::cpu::X86_64Feature::Sse3:
			return {.cpuidRegister = CpuidRegister::Leaf1Ecx, .bit = TRIVIAL_CPU_X86_64_SSE3_BIT};

		case trivial::cpu::X86_64Feature::Ssse3:
			return {.cpuidRegister = CpuidRegister::Leaf1Ecx, .bit = TRIVIAL_CPU_X86_64_SSSE3_BIT};

		case trivial::cpu::X86_64Feature::Sse41:
			return {.cpuidRegister = CpuidRegister::Leaf1Ecx, .bit = TRIVIAL_CPU_X86_64_SSE41_BIT};

		case trivial::cpu::X86_64Feature::Sse42:
			return {.cpuidRegister = CpuidRegister::Leaf1Ecx, .bit = TRIVIAL_CPU_X86_64_SSE42_BIT};

		case trivial::cpu::X86_64Feature::Sse4a:
			return {.cpuidRegister = CpuidRegister::Extended1Ecx, .bit = TRIVIAL_CPU_X86_64_SSE4A_BIT};

		case trivial::cpu::X86_64Feature::Avx:
			return {.cpuidRegister = CpuidRegister::Leaf1Ecx, .bit = TRIVIAL_CPU_X86_64_AVX_BIT};

		case trivial::cpu::X86_64Feature::Avx2:
			return {.cpuidRegister = CpuidRegister::Leaf7Ebx, .bit = TRIVIAL_CPU_X86_64_AVX2_BIT};

		case trivial::cpu::X86_64Feature::Fma:
			return {.cpuidRegister = CpuidRegister::Leaf1Ecx, .bit = TRIVIAL_CPU_X86_64_FMA_BIT};

		case trivial::cpu::X86_64Feature::F16c:
			return {.cpuidRegister = CpuidRegister::Leaf1Ecx, .bit = TRIVIAL_CPU_X86_64_F16C_BIT};

		case trivial::cpu::X86_64Feature::AvxVnni:
			return {.cpuidRegister = CpuidRegister::Leaf7Subleaf1Eax, .bit = TRIVIAL_CPU_X86_64_AVX_VNNI_BIT};

		case trivial::cpu::X86_64Feature::AvxVnniInt8:
			return {.cpuidRegister = CpuidRegister::Leaf7Subleaf1Edx, .bit = TRIVIAL_CPU_X86_64_AVX_VNNI_INT8_BIT};

		case trivial::cpu::X86_64Feature::AvxVnniInt16:
			return {.cpuidRegister = CpuidRegister::Leaf7Subleaf1Edx, .bit = TRIVIAL_CPU_X86_64_AVX_VNNI_INT16_BIT};

		case trivial::cpu::X86_64Feature::AvxIfma:
			return {.cpuidRegister = CpuidRegister::Leaf7Subleaf1Eax, .bit = TRIVIAL_CPU_X86_64_AVX_IFMA_BIT};

		case trivial::cpu::X86_64Feature::AvxNeConvert:
			return {.cpuidRegister = CpuidRegister::Leaf7Subleaf1Edx, .bit = TRIVIAL_CPU_X86_64_AVX_NE_CONVERT_BIT};

		case trivial::cpu::X86_64Feature::Xop:
			return {.cpuidRegister = CpuidRegister::Extended1Ecx, .bit = TRIVIAL_CPU_X86_64_XOP_BIT};

		case trivial::cpu::X86_64Feature::Fma4:
			return {.cpuidRegister = CpuidRegister::Extended1Ecx, .bit = TRIVIAL_CPU_X86_64_FMA4_BIT};

		case trivial::cpu::X86_64Feature::Avx512F:
			return {.cpuidRegister = CpuidRegister::Leaf7Ebx, .bit = TRIVIAL_CPU_X86_64_AVX512_F_BIT};

		case trivial::cpu::X86_64Feature::Avx512Dq:
			return {.cpuidRegister = CpuidRegister::Leaf7Ebx, .bit = TRIVIAL_CPU_X86_64_AVX512_DQ_BIT};

		case trivial::cpu::X86_64Feature::Avx512Cd:
			return {.cpuidRegister = CpuidRegister::Leaf7Ebx, .bit = TRIVIAL_CPU_X86_64_AVX512_CD_BIT};

		case trivial::cpu::X86_64Feature::Avx512Bw:
			return {.cpuidRegister = CpuidRegister::Leaf7Ebx, .bit = TRIVIAL_CPU_X86_64_AVX512_BW_BIT};

		case trivial::cpu::X86_64Feature::Avx512Vl:
			return {.cpuidRegister = CpuidRegister::Leaf7Ebx, .bit = TRIVIAL_CPU_X86_64_AVX512_VL_BIT};

		case trivial::cpu::X86_64Feature::Avx512Ifma:
			return {.cpuidRegister = CpuidRegister::Leaf7Ebx, .bit = TRIVIAL_CPU_X86_64_AVX512_IFMA_BIT};

		case trivial::cpu::X86_64Feature::Avx512Vbmi:
			return {.cpuidRegister = CpuidRegister::Leaf7Ecx, .bit = TRIVIAL_CPU_X86_64_AVX512_VBMI_BIT};

		case trivial::cpu::X86_64Feature::Avx512Vbmi2:
			return {.cpuidRegister = CpuidRegister::Leaf7Ecx, .bit = TRIVIAL_CPU_X86_64_AVX512_VBMI2_BIT};

		case trivial::cpu::X86_64Feature::Avx512Vnni:
			return {.cpuidRegister = CpuidRegister::Leaf7Ecx, .bit = TRIVIAL_CPU_X86_64_AVX512_VNNI_BIT};

		case trivial::cpu::X86_64Feature::Avx512Bitalg:
			return {.cpuidRegister = CpuidRegister::Leaf7Ecx, .bit = TRIVIAL_CPU_X86_64_AVX512_BITALG_BIT};

		case trivial::cpu::X86_64Feature::Avx512Vpopcntdq:
			return {.cpuidRegister = CpuidRegister::Leaf7Ecx, .bit = TRIVIAL_CPU_X86_64_AVX512_VPOPCNTDQ_BIT};

		case trivial::cpu::X86_64Feature::Avx512Vp2intersect:
			return {.cpuidRegister = CpuidRegister::Leaf7Edx, .bit = TRIVIAL_CPU_X86_64_AVX512_VP2INTERSECT_BIT};

		case trivial::cpu::X86_64Feature::Avx512Fp16:
			return {.cpuidRegister = CpuidRegister::Leaf7Edx, .bit = TRIVIAL_CPU_X86_64_AVX512_FP16_BIT};

		case trivial::cpu::X86_64Feature::Avx512Bf16:
			return {.cpuidRegister = CpuidRegister::Leaf7Subleaf1Eax, .bit = TRIVIAL_CPU_X86_64_AVX512_BF16_BIT};

		case trivial::cpu::X86_64Feature::Avx10V1:
			return {.cpuidRegister = CpuidRegister::Avx10Versions, .bit = TRIVIAL_CPU_X86_64_AVX10_V1_BIT};

		case trivial::cpu::X86_64Feature::Avx10V2:
			return {.cpuidRegister = CpuidRegister::Avx10Versions, .bit = TRIVIAL_CPU_X86_64_AVX10_V2_BIT};

		case trivial::cpu::X86_64Feature::ApxF:
			return {.cpuidRegister = CpuidRegister::Leaf7Subleaf1Edx, .bit = TRIVIAL_CPU_X86_64_APX_F_BIT};

		case trivial::cpu::X86_64Feature::Xsave:
			return {.cpuidRegister = CpuidRegister::Leaf1Ecx, .bit = TRIVIAL_CPU_X86_64_XSAVE_BIT};

		case trivial::cpu::X86_64Feature::Osxsave:
			return {.cpuidRegister = CpuidRegister::Leaf1Ecx, .bit = TRIVIAL_CPU_X86_64_OSXSAVE_BIT};

		case trivial::cpu::X86_64Feature::Xsaveopt:
			return {.cpuidRegister = CpuidRegister::Leaf0DSubleaf1Eax, .bit = TRIVIAL_CPU_X86_64_XSAVEOPT_BIT};

		case trivial::cpu::X86_64Feature::Xsavec:
			return {.cpuidRegister = CpuidRegister::Leaf0DSubleaf1Eax, .bit = TRIVIAL_CPU_X86_64_XSAVEC_BIT};

		case trivial::cpu::X86_64Feature::Pku:
			return {.cpuidRegister = CpuidRegister::Leaf7Ecx, .bit = TRIVIAL_CPU_X86_64_PKU_BIT};

		case trivial::cpu::X86_64Feature::Rdtscp:
			return {.cpuidRegister = CpuidRegister::Extended1Edx, .bit = TRIVIAL_CPU_X86_64_RDTSCP_BIT};

		case trivial::cpu::X86_64Feature::InvariantTsc:
			return {.cpuidRegister = CpuidRegister::Extended7Edx, .bit = TRIVIAL_CPU_X86_64_INVARIANT_TSC_BIT};

		case trivial::cpu::X86_64Feature::HypervisorPresent:
			return {.cpuidRegister = CpuidRegister::Leaf1Ecx, .bit = TRIVIAL_CPU_X86_64_HYPERVISOR_PRESENT_BIT};

		case trivial::cpu::X86_64Feature::Hybrid:
			return {.cpuidRegister = CpuidRegister::Leaf7Edx, .bit = TRIVIAL_CPU_X86_64_HYBRID_BIT};

		case trivial::cpu::X86_64Feature::Count:
			break;
	}

	TRIVIAL_UNREACHABLE();
}

[[nodiscard]] constexpr RegisterState featureRegisterState(trivial::cpu::X86_64Feature feature) noexcept {
	switch (feature) {
		case trivial::cpu::X86_64Feature::Xsave:
		case trivial::cpu::X86_64Feature::Xsaveopt:
		case trivial::cpu::X86_64Feature::Xsavec:
			return RegisterState::Xsave;

		case trivial::cpu::X86_64Feature::Sha512:
		case trivial::cpu::X86_64Feature::Sm3:
		case trivial::cpu::X86_64Feature::Sm4:
		case trivial::cpu::X86_64Feature::Vaes:
		case trivial::cpu::X86_64Feature::Vpclmulqdq:
		case trivial::cpu::X86_64Feature::Avx:
		case trivial::cpu::X86_64Feature::Avx2:
		case trivial::cpu::X86_64Feature::Fma:
		case trivial::cpu::X86_64Feature::F16c:
		case trivial::cpu::X86_64Feature::AvxVnni:
		case trivial::cpu::X86_64Feature::AvxVnniInt8:
		case trivial::cpu::X86_64Feature::AvxVnniInt16:
		case trivial::cpu::X86_64Feature::AvxIfma:
		case trivial::cpu::X86_64Feature::AvxNeConvert:
		case trivial::cpu::X86_64Feature::Xop:
		case trivial::cpu::X86_64Feature::Fma4:
			return RegisterState::Ymm;

		case trivial::cpu::X86_64Feature::Avx512F:
		case trivial::cpu::X86_64Feature::Avx512Dq:
		case trivial::cpu::X86_64Feature::Avx512Cd:
		case trivial::cpu::X86_64Feature::Avx512Bw:
		case trivial::cpu::X86_64Feature::Avx512Vl:
		case trivial::cpu::X86_64Feature::Avx512Ifma:
		case trivial::cpu::X86_64Feature::Avx512Vbmi:
		case trivial::cpu::X86_64Feature::Avx512Vbmi2:
		case trivial::cpu::X86_64Feature::Avx512Vnni:
		case trivial::cpu::X86_64Feature::Avx512Bitalg:
		case trivial::cpu::X86_64Feature::Avx512Vpopcntdq:
		case trivial::cpu::X86_64Feature::Avx512Vp2intersect:
		case trivial::cpu::X86_64Feature::Avx512Fp16:
		case trivial::cpu::X86_64Feature::Avx512Bf16:
		case trivial::cpu::X86_64Feature::Avx10V1:
		case trivial::cpu::X86_64Feature::Avx10V2:
			return RegisterState::Zmm;

		case trivial::cpu::X86_64Feature::ApxF:
			return RegisterState::Apx;

		case trivial::cpu::X86_64Feature::Cmpxchg16b:
		case trivial::cpu::X86_64Feature::Cmpccxadd:
		case trivial::cpu::X86_64Feature::RaoInt:
		case trivial::cpu::X86_64Feature::Rtm:
		case trivial::cpu::X86_64Feature::Movdiri:
		case trivial::cpu::X86_64Feature::Movdir64b:
		case trivial::cpu::X86_64Feature::Serialize:
		case trivial::cpu::X86_64Feature::EnhancedRepMovsb:
		case trivial::cpu::X86_64Feature::FastShortRepMovsb:
		case trivial::cpu::X86_64Feature::FastZeroLengthRepMovsb:
		case trivial::cpu::X86_64Feature::FastShortRepStosb:
		case trivial::cpu::X86_64Feature::FastShortRepCmpsbScasb:
		case trivial::cpu::X86_64Feature::Waitpkg:
		case trivial::cpu::X86_64Feature::Monitorx:
		case trivial::cpu::X86_64Feature::Clflushopt:
		case trivial::cpu::X86_64Feature::Clwb:
		case trivial::cpu::X86_64Feature::Cldemote:
		case trivial::cpu::X86_64Feature::Clzero:
		case trivial::cpu::X86_64Feature::Prefetchw:
		case trivial::cpu::X86_64Feature::Prefetchi:
		case trivial::cpu::X86_64Feature::Movrs:
		case trivial::cpu::X86_64Feature::Pclmulqdq:
		case trivial::cpu::X86_64Feature::Aesni:
		case trivial::cpu::X86_64Feature::Sha:
		case trivial::cpu::X86_64Feature::Gfni:
		case trivial::cpu::X86_64Feature::Rdrand:
		case trivial::cpu::X86_64Feature::Rdseed:
		case trivial::cpu::X86_64Feature::Popcnt:
		case trivial::cpu::X86_64Feature::Lzcnt:
		case trivial::cpu::X86_64Feature::Bmi1:
		case trivial::cpu::X86_64Feature::Bmi2:
		case trivial::cpu::X86_64Feature::Tbm:
		case trivial::cpu::X86_64Feature::Adx:
		case trivial::cpu::X86_64Feature::Movbe:
		case trivial::cpu::X86_64Feature::LahfSahf:
		case trivial::cpu::X86_64Feature::Rdpid:
		case trivial::cpu::X86_64Feature::Sse3:
		case trivial::cpu::X86_64Feature::Ssse3:
		case trivial::cpu::X86_64Feature::Sse41:
		case trivial::cpu::X86_64Feature::Sse42:
		case trivial::cpu::X86_64Feature::Sse4a:
		case trivial::cpu::X86_64Feature::Osxsave:
		case trivial::cpu::X86_64Feature::Pku:
		case trivial::cpu::X86_64Feature::Rdtscp:
		case trivial::cpu::X86_64Feature::InvariantTsc:
		case trivial::cpu::X86_64Feature::HypervisorPresent:
		case trivial::cpu::X86_64Feature::Hybrid:
			return RegisterState::None;
		case trivial::cpu::X86_64Feature::Count:
			break;
	}

	TRIVIAL_UNREACHABLE();
}

[[nodiscard]] constexpr trivial::cpu::FeatureSet<trivial::cpu::X86_64Feature> featurePrerequisites(
    trivial::cpu::X86_64Feature feature) noexcept {
	trivial::cpu::FeatureSet<trivial::cpu::X86_64Feature> prerequisites;

	switch (feature) {
		case trivial::cpu::X86_64Feature::Sha512:
		case trivial::cpu::X86_64Feature::Sm3:
		case trivial::cpu::X86_64Feature::Sm4:
		case trivial::cpu::X86_64Feature::Vpclmulqdq:
		case trivial::cpu::X86_64Feature::Avx2:
		case trivial::cpu::X86_64Feature::Fma:
		case trivial::cpu::X86_64Feature::F16c:
		case trivial::cpu::X86_64Feature::Xop:
		case trivial::cpu::X86_64Feature::Fma4:
			prerequisites.insert(trivial::cpu::X86_64Feature::Avx);
			return prerequisites;

		case trivial::cpu::X86_64Feature::Avx512Dq:
		case trivial::cpu::X86_64Feature::Avx512Cd:
		case trivial::cpu::X86_64Feature::Avx512Bw:
		case trivial::cpu::X86_64Feature::Avx512Vl:
		case trivial::cpu::X86_64Feature::Avx512Ifma:
		case trivial::cpu::X86_64Feature::Avx512Vbmi:
		case trivial::cpu::X86_64Feature::Avx512Vbmi2:
		case trivial::cpu::X86_64Feature::Avx512Vnni:
		case trivial::cpu::X86_64Feature::Avx512Bitalg:
		case trivial::cpu::X86_64Feature::Avx512Vpopcntdq:
		case trivial::cpu::X86_64Feature::Avx512Vp2intersect:
		case trivial::cpu::X86_64Feature::Avx512Bf16:
			prerequisites.insert(trivial::cpu::X86_64Feature::Avx512F);
			return prerequisites;

		case trivial::cpu::X86_64Feature::Avx512Fp16:
			prerequisites.insert(trivial::cpu::X86_64Feature::Avx512F);
			prerequisites.insert(trivial::cpu::X86_64Feature::Avx512Bw);
			return prerequisites;

		case trivial::cpu::X86_64Feature::Cmpxchg16b:
		case trivial::cpu::X86_64Feature::Cmpccxadd:
		case trivial::cpu::X86_64Feature::RaoInt:
		case trivial::cpu::X86_64Feature::Rtm:
		case trivial::cpu::X86_64Feature::Movdiri:
		case trivial::cpu::X86_64Feature::Movdir64b:
		case trivial::cpu::X86_64Feature::Serialize:
		case trivial::cpu::X86_64Feature::EnhancedRepMovsb:
		case trivial::cpu::X86_64Feature::FastShortRepMovsb:
		case trivial::cpu::X86_64Feature::FastZeroLengthRepMovsb:
		case trivial::cpu::X86_64Feature::FastShortRepStosb:
		case trivial::cpu::X86_64Feature::FastShortRepCmpsbScasb:
		case trivial::cpu::X86_64Feature::Waitpkg:
		case trivial::cpu::X86_64Feature::Monitorx:
		case trivial::cpu::X86_64Feature::Clflushopt:
		case trivial::cpu::X86_64Feature::Clwb:
		case trivial::cpu::X86_64Feature::Cldemote:
		case trivial::cpu::X86_64Feature::Clzero:
		case trivial::cpu::X86_64Feature::Prefetchw:
		case trivial::cpu::X86_64Feature::Prefetchi:
		case trivial::cpu::X86_64Feature::Movrs:
		case trivial::cpu::X86_64Feature::Pclmulqdq:
		case trivial::cpu::X86_64Feature::Aesni:
		case trivial::cpu::X86_64Feature::Sha:
		case trivial::cpu::X86_64Feature::Gfni:
		case trivial::cpu::X86_64Feature::Vaes:
		case trivial::cpu::X86_64Feature::Rdrand:
		case trivial::cpu::X86_64Feature::Rdseed:
		case trivial::cpu::X86_64Feature::Popcnt:
		case trivial::cpu::X86_64Feature::Lzcnt:
		case trivial::cpu::X86_64Feature::Bmi1:
		case trivial::cpu::X86_64Feature::Bmi2:
		case trivial::cpu::X86_64Feature::Tbm:
		case trivial::cpu::X86_64Feature::Adx:
		case trivial::cpu::X86_64Feature::Movbe:
		case trivial::cpu::X86_64Feature::LahfSahf:
		case trivial::cpu::X86_64Feature::Rdpid:
		case trivial::cpu::X86_64Feature::Sse3:
		case trivial::cpu::X86_64Feature::Ssse3:
		case trivial::cpu::X86_64Feature::Sse41:
		case trivial::cpu::X86_64Feature::Sse42:
		case trivial::cpu::X86_64Feature::Sse4a:
		case trivial::cpu::X86_64Feature::Avx:
		case trivial::cpu::X86_64Feature::AvxVnni:
		case trivial::cpu::X86_64Feature::AvxVnniInt8:
		case trivial::cpu::X86_64Feature::AvxVnniInt16:
		case trivial::cpu::X86_64Feature::AvxIfma:
		case trivial::cpu::X86_64Feature::AvxNeConvert:
		case trivial::cpu::X86_64Feature::Avx512F:
		case trivial::cpu::X86_64Feature::Avx10V1:
		case trivial::cpu::X86_64Feature::Avx10V2:
		case trivial::cpu::X86_64Feature::ApxF:
		case trivial::cpu::X86_64Feature::Xsave:
		case trivial::cpu::X86_64Feature::Osxsave:
		case trivial::cpu::X86_64Feature::Xsaveopt:
		case trivial::cpu::X86_64Feature::Xsavec:
		case trivial::cpu::X86_64Feature::Pku:
		case trivial::cpu::X86_64Feature::Rdtscp:
		case trivial::cpu::X86_64Feature::InvariantTsc:
		case trivial::cpu::X86_64Feature::HypervisorPresent:
		case trivial::cpu::X86_64Feature::Hybrid:
			return prerequisites;
		case trivial::cpu::X86_64Feature::Count:
			break;
	}

	TRIVIAL_UNREACHABLE();
}

static_assert(
    [] {
	    for (std::size_t i = 0; i < static_cast<std::size_t>(trivial::cpu::X86_64Feature::Count); ++i) {
		    const trivial::cpu::FeatureSet<trivial::cpu::X86_64Feature> kPrerequisites
		        = featurePrerequisites(static_cast<trivial::cpu::X86_64Feature>(i));

		    for (std::size_t j = 0; j < static_cast<std::size_t>(trivial::cpu::X86_64Feature::Count); ++j) {
			    const trivial::cpu::X86_64Feature kOther = static_cast<trivial::cpu::X86_64Feature>(j);

			    if (kPrerequisites.contains(kOther) && !kPrerequisites.containsAll(featurePrerequisites(kOther))) {
				    return false;
			    }
		    }
	    }

	    return true;
    }(),
    "Every feature should list the prerequisites of its prerequisites too");

static_assert(
    [] {
	    const std::uint32_t kCoffeeLake = 0x000906EA;
	    const std::uint32_t kStepping = 0xA;
	    const std::uint32_t kModel = 0xE;
	    const std::uint32_t kExtendedModel = 0x9;

	    return trivial::cpu::extractBits(kCoffeeLake,
		                                 TRIVIAL_CPU_X86_64_SIGNATURE_STEPPING_HIGH_BIT,
		                                 TRIVIAL_CPU_X86_64_SIGNATURE_STEPPING_LOW_BIT)
	               == kStepping
	           && trivial::cpu::extractBits(kCoffeeLake,
	                                        TRIVIAL_CPU_X86_64_SIGNATURE_MODEL_HIGH_BIT,
	                                        TRIVIAL_CPU_X86_64_SIGNATURE_MODEL_LOW_BIT)
	                  == kModel
	           && trivial::cpu::extractBits(kCoffeeLake,
	                                        TRIVIAL_CPU_X86_64_SIGNATURE_FAMILY_HIGH_BIT,
	                                        TRIVIAL_CPU_X86_64_SIGNATURE_FAMILY_LOW_BIT)
	                  == TRIVIAL_CPU_X86_64_FAMILY_06H
	           && trivial::cpu::extractBits(kCoffeeLake,
	                                        TRIVIAL_CPU_X86_64_SIGNATURE_EXTENDED_MODEL_HIGH_BIT,
	                                        TRIVIAL_CPU_X86_64_SIGNATURE_EXTENDED_MODEL_LOW_BIT)
	                  == kExtendedModel
	           && trivial::cpu::extractBits(kCoffeeLake,
	                                        TRIVIAL_CPU_X86_64_SIGNATURE_EXTENDED_FAMILY_HIGH_BIT,
	                                        TRIVIAL_CPU_X86_64_SIGNATURE_EXTENDED_FAMILY_LOW_BIT)
	                  == 0;
    }(),
    "The CPUID signature fields should match their bit positions (Coffee Lake, 000906EAH)");

struct CpuidResult {
	std::uint32_t eax;
	std::uint32_t ebx;
	std::uint32_t ecx;
	std::uint32_t edx;
};

[[nodiscard]] CpuidResult cpuid(std::uint32_t leaf, std::uint32_t subleaf) noexcept {
#if TRIVIAL_COMPILER_MSVC || TRIVIAL_COMPILER_CLANG_CL
	std::array<int, 4> registers{};
	__cpuidex(registers.data(), static_cast<int>(leaf), static_cast<int>(subleaf));
	return std::bit_cast<CpuidResult>(registers);
#else
	CpuidResult result{};
	__cpuid_count(leaf, subleaf, result.eax, result.ebx, result.ecx, result.edx);
	return result;
#endif // Compiler check
}

using CpuidRegisters = std::array<std::uint32_t, static_cast<std::size_t>(CpuidRegister::Count)>;

[[nodiscard]] CpuidRegisters readFeatureRegisters(std::uint32_t maxLeaf, std::uint32_t maxExtendedLeaf) noexcept {
	CpuidRegisters registers{};

	if (maxLeaf >= TRIVIAL_CPU_X86_64_LEAF_FEATURES) {
		const CpuidResult kLeaf1 = cpuid(TRIVIAL_CPU_X86_64_LEAF_FEATURES, 0);
		registers[static_cast<std::size_t>(CpuidRegister::Leaf1Eax)] = kLeaf1.eax;
		registers[static_cast<std::size_t>(CpuidRegister::Leaf1Ecx)] = kLeaf1.ecx;
	}

	if (maxLeaf >= TRIVIAL_CPU_X86_64_LEAF_STRUCTURED_FEATURES) {
		const CpuidResult kLeaf7 = cpuid(TRIVIAL_CPU_X86_64_LEAF_STRUCTURED_FEATURES, 0);
		registers[static_cast<std::size_t>(CpuidRegister::Leaf7Ebx)] = kLeaf7.ebx;
		registers[static_cast<std::size_t>(CpuidRegister::Leaf7Ecx)] = kLeaf7.ecx;
		registers[static_cast<std::size_t>(CpuidRegister::Leaf7Edx)] = kLeaf7.edx;

		if (kLeaf7.eax >= 1) {
			const CpuidResult kLeaf7Subleaf1 = cpuid(TRIVIAL_CPU_X86_64_LEAF_STRUCTURED_FEATURES, 1);
			registers[static_cast<std::size_t>(CpuidRegister::Leaf7Subleaf1Eax)] = kLeaf7Subleaf1.eax;
			registers[static_cast<std::size_t>(CpuidRegister::Leaf7Subleaf1Edx)] = kLeaf7Subleaf1.edx;

			if (((kLeaf7Subleaf1.edx >> TRIVIAL_CPU_X86_64_AVX10_BIT) & 1U) != 0
			    && maxLeaf >= TRIVIAL_CPU_X86_64_LEAF_AVX10) {
				const std::uint32_t kLeaf24Ebx = cpuid(TRIVIAL_CPU_X86_64_LEAF_AVX10, 0).ebx;
				const std::uint8_t kVersion = static_cast<std::uint8_t>(kLeaf24Ebx);
				registers[static_cast<std::size_t>(CpuidRegister::Leaf24Ebx)] = kLeaf24Ebx;
				registers[static_cast<std::size_t>(CpuidRegister::Avx10Versions)]
				    = (kVersion >= 1 ? 1U << TRIVIAL_CPU_X86_64_AVX10_V1_BIT : 0U)
				      | (kVersion >= 2 ? 1U << TRIVIAL_CPU_X86_64_AVX10_V2_BIT : 0U);
			}
		}
	}

	if (maxLeaf >= TRIVIAL_CPU_X86_64_LEAF_EXTENDED_STATE) {
		registers[static_cast<std::size_t>(CpuidRegister::Leaf0DSubleaf1Eax)]
		    = cpuid(TRIVIAL_CPU_X86_64_LEAF_EXTENDED_STATE, 1).eax;
	}

	if (maxExtendedLeaf >= TRIVIAL_CPU_X86_64_LEAF_EXTENDED_FEATURES) {
		const CpuidResult kExtended1 = cpuid(TRIVIAL_CPU_X86_64_LEAF_EXTENDED_FEATURES, 0);
		registers[static_cast<std::size_t>(CpuidRegister::Extended1Ecx)] = kExtended1.ecx;
		registers[static_cast<std::size_t>(CpuidRegister::Extended1Edx)] = kExtended1.edx;
	}

	if (maxExtendedLeaf >= TRIVIAL_CPU_X86_64_LEAF_POWER_MANAGEMENT) {
		registers[static_cast<std::size_t>(CpuidRegister::Extended7Edx)]
		    = cpuid(TRIVIAL_CPU_X86_64_LEAF_POWER_MANAGEMENT, 0).edx;
	}

	if (maxExtendedLeaf >= TRIVIAL_CPU_X86_64_LEAF_EXTENDED_CAPACITY) {
		registers[static_cast<std::size_t>(CpuidRegister::Extended8Ebx)]
		    = cpuid(TRIVIAL_CPU_X86_64_LEAF_EXTENDED_CAPACITY, 0).ebx;
	}

	if (maxExtendedLeaf >= TRIVIAL_CPU_X86_64_LEAF_EXTENDED_FEATURES_2) {
		registers[static_cast<std::size_t>(CpuidRegister::Extended21Eax)]
		    = cpuid(TRIVIAL_CPU_X86_64_LEAF_EXTENDED_FEATURES_2, 0).eax;
	}

	return registers;
}

[[nodiscard]] bool isXcr0StateEnabled(RegisterState state, std::uint64_t xcr0, bool osXsave) noexcept {
	const bool kYmmState = osXsave && (xcr0 & TRIVIAL_CPU_X86_64_XCR0_AVX_STATE) == TRIVIAL_CPU_X86_64_XCR0_AVX_STATE;

	switch (state) {
		case RegisterState::None:
			return true;
		case RegisterState::Xsave:
			return osXsave;
		case RegisterState::Ymm:
			return kYmmState;
		case RegisterState::Zmm:
			return kYmmState
			       && (TRIVIAL_PLATFORM_MACOS
			           || (xcr0 & TRIVIAL_CPU_X86_64_XCR0_AVX512_STATE) == TRIVIAL_CPU_X86_64_XCR0_AVX512_STATE);
		case RegisterState::Apx:
			return osXsave && (xcr0 & TRIVIAL_CPU_X86_64_XCR0_APX_STATE) == TRIVIAL_CPU_X86_64_XCR0_APX_STATE;
		case RegisterState::Count:
			break;
	}

	TRIVIAL_UNREACHABLE();
}

void probeFeatures(const CpuidRegisters& registers) noexcept {
	trivial::cpu::FeatureSet<trivial::cpu::X86_64Feature> reported;

	for (std::size_t i = 0; i < static_cast<std::size_t>(trivial::cpu::X86_64Feature::Count); ++i) {
		const trivial::cpu::X86_64Feature kFeature = static_cast<trivial::cpu::X86_64Feature>(i);
		const CpuidBit kSource = cpuidBit(kFeature);

		if (((registers[static_cast<std::size_t>(kSource.cpuidRegister)] >> kSource.bit) & 1U) != 0) {
			reported.insert(kFeature);
		}
	}

	if (((registers[static_cast<std::size_t>(CpuidRegister::Extended21Eax)] >> TRIVIAL_CPU_X86_64_AMD_PREFETCHI_BIT)
	     & 1U)
	    != 0) {
		reported.insert(trivial::cpu::X86_64Feature::Prefetchi);
	}

	const bool kOsXsave = reported.contains(trivial::cpu::X86_64Feature::Osxsave);
	std::uint64_t xcr0 = 0;

	if (kOsXsave) {
#if TRIVIAL_COMPILER_MSVC
		xcr0 = _xgetbv(0);
#else
		std::uint32_t low = 0;
		std::uint32_t high = 0;
		__asm__ volatile("xgetbv" : "=a"(low), "=d"(high) : "c"(0)); // NOLINT(portability-no-assembler)
		xcr0 = (std::uint64_t{high} << TRIVIAL_CPU_X86_64_XCR0_HIGH_SHIFT) | low;
#endif // Compiler check
	}

	const bool kIsIntel
	    = std::string_view{trivial::cpu::detail::g_detectedCapabilities.identity.vendor.data()} == "GenuineIntel";
	const bool kBmiErratum = kIsIntel && !reported.contains(trivial::cpu::X86_64Feature::Avx);
	trivial::cpu::FeatureSet<trivial::cpu::X86_64Feature> usable;

	for (std::size_t i = 0; i < static_cast<std::size_t>(trivial::cpu::X86_64Feature::Count); ++i) {
		const trivial::cpu::X86_64Feature kFeature = static_cast<trivial::cpu::X86_64Feature>(i);
		const bool kErratum
		    = kBmiErratum
		      && (kFeature == trivial::cpu::X86_64Feature::Bmi1 || kFeature == trivial::cpu::X86_64Feature::Bmi2);

		if (reported.contains(kFeature) && isXcr0StateEnabled(featureRegisterState(kFeature), xcr0, kOsXsave)
		    && !kErratum) {
			usable.insert(kFeature);
		}
	}

	trivial::cpu::insertFeaturesWithPrerequisites<featurePrerequisites>(usable);
}

} // namespace

namespace trivial::cpu {

void probeIdentity() noexcept {
	detail::Identity& identity = detail::g_detectedCapabilities.identity;

	const CpuidResult kVendorLeaf = cpuid(TRIVIAL_CPU_X86_64_LEAF_VENDOR, 0);
	const std::array<std::uint32_t, 3> kVendorWords{kVendorLeaf.ebx, kVendorLeaf.edx, kVendorLeaf.ecx};
	static_assert(sizeof(kVendorWords) < sizeof(VendorString), "Vendor string should fit with its terminator");
	std::memcpy(identity.vendor.data(), kVendorWords.data(), sizeof(kVendorWords));
	identity.vendor[sizeof(kVendorWords)] = '\0';

	const std::uint32_t kMaxLeaf = kVendorLeaf.eax;
	const std::uint32_t kMaxExtendedLeaf = cpuid(TRIVIAL_CPU_X86_64_LEAF_EXTENDED_MAX, 0).eax;

	const CpuidRegisters kRegisters = readFeatureRegisters(kMaxLeaf, kMaxExtendedLeaf);

	probeFeatures(kRegisters);

	if (kMaxLeaf >= TRIVIAL_CPU_X86_64_LEAF_FEATURES) {
		const std::uint32_t kSignature = kRegisters[static_cast<std::size_t>(CpuidRegister::Leaf1Eax)];
		const std::uint32_t kFamilyId = extractBits(kSignature,
		                                            TRIVIAL_CPU_X86_64_SIGNATURE_FAMILY_HIGH_BIT,
		                                            TRIVIAL_CPU_X86_64_SIGNATURE_FAMILY_LOW_BIT);
		std::uint32_t family = kFamilyId;
		std::uint32_t model = extractBits(kSignature,
		                                  TRIVIAL_CPU_X86_64_SIGNATURE_MODEL_HIGH_BIT,
		                                  TRIVIAL_CPU_X86_64_SIGNATURE_MODEL_LOW_BIT);

		if (kFamilyId == TRIVIAL_CPU_X86_64_FAMILY_0FH) {
			family += extractBits(kSignature,
			                      TRIVIAL_CPU_X86_64_SIGNATURE_EXTENDED_FAMILY_HIGH_BIT,
			                      TRIVIAL_CPU_X86_64_SIGNATURE_EXTENDED_FAMILY_LOW_BIT);
		}

		if (kFamilyId == TRIVIAL_CPU_X86_64_FAMILY_06H || kFamilyId == TRIVIAL_CPU_X86_64_FAMILY_0FH) {
			model += extractBits(kSignature,
			                     TRIVIAL_CPU_X86_64_SIGNATURE_EXTENDED_MODEL_HIGH_BIT,
			                     TRIVIAL_CPU_X86_64_SIGNATURE_EXTENDED_MODEL_LOW_BIT)
			         << 4U;
		}

		identity.architectureInfo.family = static_cast<std::uint16_t>(family);
		identity.architectureInfo.model = static_cast<std::uint8_t>(model);
		identity.architectureInfo.stepping
		    = static_cast<std::uint8_t>(extractBits(kSignature,
		                                            TRIVIAL_CPU_X86_64_SIGNATURE_STEPPING_HIGH_BIT,
		                                            TRIVIAL_CPU_X86_64_SIGNATURE_STEPPING_LOW_BIT));
	}

	if (identity.features.contains(X86_64Feature::Avx10V1)) {
		identity.architectureInfo.avx10Version
		    = static_cast<std::uint8_t>(kRegisters[static_cast<std::size_t>(CpuidRegister::Leaf24Ebx)]);
	}

	if (kMaxExtendedLeaf >= TRIVIAL_CPU_X86_64_LEAF_BRAND_LAST) {
		const std::array<CpuidResult, 3> kBrandLeaves{
		    cpuid(TRIVIAL_CPU_X86_64_LEAF_BRAND_FIRST, 0),
		    cpuid(TRIVIAL_CPU_X86_64_LEAF_BRAND_FIRST + 1, 0),
		    cpuid(TRIVIAL_CPU_X86_64_LEAF_BRAND_LAST, 0),
		};
		static_assert(sizeof(kBrandLeaves) < sizeof(BrandString), "Brand string should fit with its terminator");

		BrandString& brand = identity.brand;
		std::memcpy(brand.data(), kBrandLeaves.data(), sizeof(kBrandLeaves));
		brand[sizeof(kBrandLeaves)] = '\0';
		const std::size_t kLeadingSpaces = std::strspn(brand.data(), " ");
		std::memmove(brand.data(), &brand[kLeadingSpaces], std::strlen(&brand[kLeadingSpaces]) + 1);
	}
}

} // namespace trivial::cpu

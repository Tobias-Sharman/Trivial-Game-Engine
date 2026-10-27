#ifndef TRIVIAL_CORE_CPU_CPU_FEATURES_ARM64_H
#define TRIVIAL_CORE_CPU_CPU_FEATURES_ARM64_H

#include <trivial/core/platform.h>

#if TRIVIAL_ARCH_ARM64
#include <cstdint>

#include <trivial/core/compiler.h>
#include <trivial/core/config.h>
#include <trivial/core/cpu/cpu_feature_set.h>

#if TRIVIAL_ENABLE_LOGGING
#include <trivial/core/assert.h>
#endif // TRIVIAL_ENABLE_LOGGING

#if (TRIVIAL_CPU_ARM64_APPLE_M1 != 0) && (TRIVIAL_CPU_ARM64_APPLE_M1 != 1)
#error "TRIVIAL_CPU_ARM64_APPLE_M1 must be 0 or 1"
#endif

#if TRIVIAL_CPU_ARM64_APPLE_M1                                                                                         \
    && (!defined(__ARM_FEATURE_ATOMICS) || !defined(__ARM_FEATURE_DOTPROD) || !defined(__ARM_FEATURE_SHA3))
#error "Compiler target is below apple-m1, the flags from trivial_apply_cpu_level() are missing"
#endif

namespace trivial::cpu {

enum class Arm64Feature : std::uint8_t {
	Crc32,     // CRC32 instructions, for checksums and fast hashing (LLVM crc)
	Aes,       // AES rounds, for encryption and fast hashing
	Pmull,     // 64-bit polynomial multiply, for fast CRC and AES-GCM (part of LLVM aes)
	Sha1,      // SHA-1 hashing (part of LLVM sha2)
	Sha256,    // SHA-256 hashing (part of LLVM sha2)
	Sha3,      // Three-way XOR and rotates, for SHA-3 and bit mixing
	Sha512,    // SHA-512 hashing
	Sm3,       // SM3 hashing, a Chinese standard (part of LLVM sm4)
	Sm4,       // SM4 encryption, a Chinese standard (LLVM sm4)
	Lse,       // Single instruction atomics (CAS, LDADD), faster under contention (Linux ATOMICS)
	Lse2,      // Atomic 16-byte aligned loads and stores, for wide atomics (Linux USCAT)
	Rdm,       // Rounding doubling multiply-accumulate, for fixed point maths (Linux ASIMDRDM)
	Lrcpc,     // Weaker acquire loads (LDAPR), for cheaper acquire ordering (LLVM rcpc)
	Lrcpc2,    // RCPC loads with offsets, for fewer address calculations (LLVM rcpc-immo and rcpc2, Linux ILRCPC)
	DotProd,   // Int8 dot products, for inference and pixel maths (Linux ASIMDDP)
	Fp16,      // Half precision arithmetic, for compact maths (Linux FPHP and ASIMDHP)
	Fhm,       // Half precision multiply-accumulate into single precision (LLVM fp16fml, Linux ASIMDFHM)
	Jscvt,     // JavaScript style float to int conversion
	Fcma,      // Complex number multiply-add, for FFTs and signal processing
	Frintts,   // Round floats to the 32 and 64-bit integer range, for safe casts (LLVM fptoint, Linux FRINT)
	FlagM,     // Condition flag manipulation, for branchless code
	FlagM2,    // Further condition flag manipulation (LLVM altnzcv)
	Sb,        // Speculation barrier, for Spectre mitigations
	Ssbs,      // Speculative store bypass control, for Spectre mitigations
	Dit,       // Data independent timing, for constant time crypto
	Dpb,       // Cache clean to persistence, for persistent memory (LLVM ccpp, Linux DCPOP)
	Dpb2,      // Cache clean to deep persistence, for persistent memory (LLVM ccdp, Linux DCPODP)
	Bti,       // Branch target identification, for control flow protection
	PAuth,     // Pointer authentication, for return address protection (Linux PACA and PACG)
	I8mm,      // Int8 matrix multiply, for inference
	Bf16,      // Bfloat16 maths, for machine learning
	Ebf16,     // Extended bfloat16 rounding behaviour
	Mte,       // Memory tagging, for catching use after free and overflows
	Sve,       // Scalable vectors whose length is set by the hardware
	Sve2,      // SVE for integer and signal work, a superset of NEON
	SveI8mm,   // SVE int8 matrix multiply
	SveBf16,   // SVE bfloat16 maths
	Sme,       // Scalable matrix outer products and streaming mode
	Sme2,      // SME with multi-vector instructions
	SmeF64F64, // SME double precision outer products
	SmeI16I64, // SME int16 to int64 outer products
	Cssc,      // Absolute, popcount, min and max on general registers
	Wfxt,      // Waits with a timeout, for low power spin loops
	Mops,      // Memory copy and set instructions, for fast copies
	Hbc,       // Conditional branches with prediction hints
	Afp,       // Alternate float handling of denormals and NaNs
	Rpres,     // More precise reciprocal estimates
	Ecv,       // Enhanced counter virtualisation, for virtual machine timers

	Count,
};

struct Arm64MainId {
	std::uint16_t part = 0;
	std::uint8_t implementer = 0;
	std::uint8_t variant = 0;
	// std::uint8_t architecture = 0; // Always the same so no point in keeping
	std::uint8_t revision = 0;
};

#if TRIVIAL_ENABLE_LOGGING
[[nodiscard]] TRIVIAL_COLD constexpr const char* featureName(Arm64Feature feature) noexcept {
	switch (feature) {
		case Arm64Feature::Crc32:
			return "FEAT_CRC32";
		case Arm64Feature::Aes:
			return "FEAT_AES";
		case Arm64Feature::Pmull:
			return "FEAT_PMULL";
		case Arm64Feature::Sha1:
			return "FEAT_SHA1";
		case Arm64Feature::Sha256:
			return "FEAT_SHA256";
		case Arm64Feature::Sha3:
			return "FEAT_SHA3";
		case Arm64Feature::Sha512:
			return "FEAT_SHA512";
		case Arm64Feature::Sm3:
			return "FEAT_SM3";
		case Arm64Feature::Sm4:
			return "FEAT_SM4";
		case Arm64Feature::Lse:
			return "FEAT_LSE";
		case Arm64Feature::Lse2:
			return "FEAT_LSE2";
		case Arm64Feature::Rdm:
			return "FEAT_RDM";
		case Arm64Feature::Lrcpc:
			return "FEAT_LRCPC";
		case Arm64Feature::Lrcpc2:
			return "FEAT_LRCPC2";
		case Arm64Feature::DotProd:
			return "FEAT_DotProd";
		case Arm64Feature::Fp16:
			return "FEAT_FP16";
		case Arm64Feature::Fhm:
			return "FEAT_FHM";
		case Arm64Feature::Jscvt:
			return "FEAT_JSCVT";
		case Arm64Feature::Fcma:
			return "FEAT_FCMA";
		case Arm64Feature::Frintts:
			return "FEAT_FRINTTS";
		case Arm64Feature::FlagM:
			return "FEAT_FlagM";
		case Arm64Feature::FlagM2:
			return "FEAT_FlagM2";
		case Arm64Feature::Sb:
			return "FEAT_SB";
		case Arm64Feature::Ssbs:
			return "FEAT_SSBS";
		case Arm64Feature::Dit:
			return "FEAT_DIT";
		case Arm64Feature::Dpb:
			return "FEAT_DPB";
		case Arm64Feature::Dpb2:
			return "FEAT_DPB2";
		case Arm64Feature::Bti:
			return "FEAT_BTI";
		case Arm64Feature::PAuth:
			return "FEAT_PAuth";
		case Arm64Feature::I8mm:
			return "FEAT_I8MM";
		case Arm64Feature::Bf16:
			return "FEAT_BF16";
		case Arm64Feature::Ebf16:
			return "FEAT_EBF16";
		case Arm64Feature::Mte:
			return "FEAT_MTE";
		case Arm64Feature::Sve:
			return "FEAT_SVE";
		case Arm64Feature::Sve2:
			return "FEAT_SVE2";
		case Arm64Feature::SveI8mm:
			return "HWCAP2_SVEI8MM";
		case Arm64Feature::SveBf16:
			return "HWCAP2_SVEBF16";
		case Arm64Feature::Sme:
			return "FEAT_SME";
		case Arm64Feature::Sme2:
			return "FEAT_SME2";
		case Arm64Feature::SmeF64F64:
			return "FEAT_SME_F64F64";
		case Arm64Feature::SmeI16I64:
			return "FEAT_SME_I16I64";
		case Arm64Feature::Cssc:
			return "FEAT_CSSC";
		case Arm64Feature::Wfxt:
			return "FEAT_WFxT";
		case Arm64Feature::Mops:
			return "FEAT_MOPS";
		case Arm64Feature::Hbc:
			return "FEAT_HBC";
		case Arm64Feature::Afp:
			return "FEAT_AFP";
		case Arm64Feature::Rpres:
			return "FEAT_RPRES";
		case Arm64Feature::Ecv:
			return "FEAT_ECV";
		case Arm64Feature::Count:
			break;
	}

	TRIVIAL_UNREACHABLE();
}
#endif // TRIVIAL_ENABLE_LOGGING

namespace detail {

[[nodiscard]] consteval FeatureSet<Arm64Feature> guaranteedFeatures() noexcept {
	FeatureSet<Arm64Feature> features;

	if constexpr (TRIVIAL_CPU_ARM64_APPLE_M1) {
		features.insert(Arm64Feature::Lse);
		features.insert(Arm64Feature::Lse2);
		features.insert(Arm64Feature::Lrcpc);
		features.insert(Arm64Feature::Lrcpc2);
		features.insert(Arm64Feature::Rdm);
		features.insert(Arm64Feature::DotProd);
		features.insert(Arm64Feature::Fp16);
		features.insert(Arm64Feature::Fhm);
		features.insert(Arm64Feature::Crc32);
		features.insert(Arm64Feature::Aes);
		features.insert(Arm64Feature::Pmull);
		features.insert(Arm64Feature::Sha1);
		features.insert(Arm64Feature::Sha256);
		features.insert(Arm64Feature::Sha3);
		features.insert(Arm64Feature::Sha512);
		features.insert(Arm64Feature::Jscvt);
		features.insert(Arm64Feature::Fcma);
		features.insert(Arm64Feature::Frintts);
		features.insert(Arm64Feature::FlagM);
		features.insert(Arm64Feature::FlagM2);
		features.insert(Arm64Feature::Sb);
		features.insert(Arm64Feature::Dit);
		features.insert(Arm64Feature::Dpb);
		features.insert(Arm64Feature::Dpb2);
		features.insert(Arm64Feature::PAuth);
	}

	return features;
}

} // namespace detail

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr bool isGuaranteed(Arm64Feature feature) noexcept {
	return detail::guaranteedFeatures().contains(feature);
}

} // namespace trivial::cpu

#endif // TRIVIAL_ARCH_ARM64

#endif // TRIVIAL_CORE_CPU_CPU_FEATURES_ARM64_H

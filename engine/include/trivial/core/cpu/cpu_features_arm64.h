#ifndef TRIVIAL_CORE_CPU_CPU_FEATURES_ARM64_H
#define TRIVIAL_CORE_CPU_CPU_FEATURES_ARM64_H

#include <trivial/core/platform.h>

#if TRIVIAL_ARCH_ARM64
#include <array>
#include <cstdint>

#include <trivial/core/compiler.h>
#include <trivial/core/config.h>
#include <trivial/core/cpu/cpu_feature_set.h>

#if TRIVIAL_ENABLE_LOGGING
#include <trivial/core/assert.h>
#endif // TRIVIAL_ENABLE_LOGGING

#if TRIVIAL_CPU_ARM64_APPLE_M1                                                                                         \
    && (!defined(__ARM_FEATURE_ATOMICS) || !defined(__ARM_FEATURE_DOTPROD) || !defined(__ARM_FEATURE_SHA3))
#error "Compiler target is below apple-m1, the flags from trivial_apply_cpu_version() are missing"
#endif

namespace trivial::cpu {

enum class Arm64Feature : std::uint8_t {
	// Atomics, memory ordering, copies and waits
	Lse,    // Single instruction atomics (CAS, LDADD), faster under contention (Linux ATOMICS)
	Lse2,   // Atomic 16-byte aligned loads and stores, for wide atomics (Linux USCAT)
	Lse128, // 128-bit atomic bit clear, set and swap, for wide lock-free state
	Lsfe,   // Atomic floating point add, min and max in memory, for parallel accumulation
	Lrcpc,  // Weaker acquire loads (LDAPR), for cheaper acquire ordering (LLVM rcpc)
	Lrcpc2, // RCPC loads with offsets, for fewer address calculations (LLVM rcpc-immo and rcpc2, Linux ILRCPC)
	Lrcpc3, // Further acquire and release loads and stores, including pairs and SIMD registers (LLVM rcpc3)
	Mops,   // Memory copy and set instructions, for fast copies
	Rprfm,  // Range prefetch hints, for streaming over large buffers
	Wfxt,   // Waits with a timeout, for low power spin loops

	// Checksums, hashing, encryption and random numbers
	Crc32,  // CRC32 instructions, for checksums and fast hashing (LLVM crc)
	Aes,    // AES rounds, for encryption and fast hashing
	Pmull,  // 64-bit polynomial multiply, for fast CRC and AES-GCM (part of LLVM aes)
	Sha1,   // SHA-1 hashing (part of LLVM sha2)
	Sha256, // SHA-256 hashing (part of LLVM sha2)
	Sha3,   // Three-way XOR and rotates, for SHA-3 and bit mixing
	Sha512, // SHA-512 hashing
	Sm3,    // SM3 hashing, a Chinese standard (part of LLVM sm4)
	Sm4,    // SM4 encryption, a Chinese standard (LLVM sm4)
	Rng,    // Hardware random numbers (RNDR), for seeding (LLVM rand)

	// General purpose instructions
	Cssc,   // Absolute, popcount, min and max on general registers
	FlagM,  // Condition flag manipulation, for branchless code
	FlagM2, // Further condition flag manipulation (LLVM altnzcv)
	Jscvt,  // JavaScript style float to int conversion
	Hbc,    // Conditional branches with prediction hints
	Cmpbr,  // Compare and branch in one instruction

	// Floating point conversions and behaviour
	Frintts,          // Round floats to the 32 and 64-bit integer range, for safe casts (LLVM fptoint, Linux FRINT)
	Fprcvt,           // Float to int conversions between general and SIMD registers
	Afp,              // Alternate float handling of denormals and NaNs
	Rpres,            // More precise reciprocal estimates
	FpSyncExceptions, // Trapping floating point exceptions, for catching NaN and overflow sources

	// Advanced SIMD (NEON) maths
	Rdm,       // Rounding doubling multiply-accumulate, for fixed point maths (Linux ASIMDRDM)
	DotProd,   // Int8 dot products, for inference and pixel maths (Linux ASIMDDP)
	Fp16,      // Half precision arithmetic, for compact maths (Linux FPHP and ASIMDHP)
	Fhm,       // Half precision multiply-accumulate into single precision (LLVM fp16fml, Linux ASIMDFHM)
	F16f32dot, // Half precision dot products into single precision
	F16f32mm,  // Half precision matrix multiply into single precision
	F16mm,     // Half precision matrix multiply
	Fcma,      // Complex number multiply-add, for FFTs and signal processing
	I8mm,      // Int8 matrix multiply, for inference
	Bf16,      // Bfloat16 maths, for machine learning
	Ebf16,     // Extended bfloat16 rounding behaviour
	Lut,       // Table lookups (LUTI2, LUTI4), for small lookup tables
	Faminmax,  // Absolute minimum and maximum, for range and normalisation work

	// FP8, every instruction of which reads the FP8 mode register
	Fpmr,    // FP8 mode register, needed by every FP8 instruction
	Fp8,     // FP8 conversions, for compact machine learning data
	Fp8Fma,  // FP8 multiply-add
	Fp8Dot4, // FP8 four way dot products
	Fp8Dot2, // FP8 two way dot products
	F8f16mm, // FP8 matrix multiply into half precision (Linux F8MM4)
	F8f32mm, // FP8 matrix multiply into single precision (Linux F8MM8)

	// SVE outside streaming mode
	Sve,         // Scalable vectors whose length is set by the hardware
	Sve2,        // SVE for integer and signal work, a superset of NEON
	Sve2p1,      // SVE2.1 instructions
	Sve2p2,      // SVE2.2 instructions
	Sve2p3,      // SVE2.3 instructions
	SveI8mm,     // SVE int8 matrix multiply
	SveBf16,     // SVE bfloat16 maths
	SveEbf16,    // SVE extended bfloat16 rounding behaviour
	SveF32mm,    // SVE single precision matrix multiply
	SveF64mm,    // SVE double precision matrix multiply
	SveF16f32mm, // SVE half precision matrix multiply into single precision (Linux SVE_F16MM)
	SveB16b16,   // SVE bfloat16 arithmetic
	SveB16mm,    // SVE bfloat16 matrix multiply
	SveBfscale,  // SVE bfloat16 scaling
	SveAes,      // SVE AES rounds (LLVM sve2-aes)
	SvePmull128, // SVE 128-bit polynomial multiply (part of LLVM sve2-aes)
	SveAes2,     // SVE multi-vector AES and 128-bit polynomial multiply
	SveBitperm,  // SVE bit deposit, extract and group (LLVM sve2-bitperm)
	SveSha3,     // SVE SHA-3 rotates (LLVM sve2-sha3)
	SveSm4,      // SVE SM4 encryption (LLVM sve2-sm4)
	SvePerm256,  // SVE 128-bit element permutes without F64MM (Linux SVE_ELTPERM)
	SveLut6,     // SVE six-bit table lookups

	// SME, which runs in streaming mode
	Sme,       // Scalable matrix outer products and streaming mode
	Sme2,      // SME with multi-vector instructions
	Sme2p1,    // SME2.1 instructions
	Sme2p2,    // SME2.2 instructions
	Sme2p3,    // SME2.3 instructions
	SmeF64F64, // SME double precision outer products
	SmeI16I64, // SME int16 to int64 outer products
	SmeF16F16, // SME half precision outer products
	SmeB16B16, // SME bfloat16 arithmetic
	SmeF8F16,  // SME FP8 to half precision outer products
	SmeF8F32,  // SME FP8 to single precision outer products
	SmeLutv2,  // SME four register table lookups
	SmeLut6,   // SME six-bit table lookups
	SmeTmop,   // SME sparse outer products (Linux SME_STMOP)
	SmeMop4,   // SME quarter tile outer products (Linux SME_SMOP4)
	SmeFa64,   // The full A64 instruction set in streaming mode

	// SVE instructions inside streaming mode
	SsveAes,     // SVE AES in streaming mode (Linux SME_AES)
	SsveBitperm, // SVE bit permutes in streaming mode (Linux SME_SBITPERM)
	SsveFexpa,   // SVE exponential acceleration in streaming mode (Linux SME_SFEXPA)
	SsveFp8Fma,  // FP8 multiply-add in streaming mode
	SsveFp8Dot4, // FP8 four way dot products in streaming mode
	SsveFp8Dot2, // FP8 two way dot products in streaming mode
	SsveF8f16mm, // FP8 matrix multiply into half precision in streaming mode
	SsveF8f32mm, // FP8 matrix multiply into single precision in streaming mode

	// Memory tagging and permission overlays
	Mte,          // Memory tagging, for catching use after free and overflows
	Mte2,         // Memory tag checking on loads and stores
	Mte3,         // Asymmetric tag checking, synchronous reads and asynchronous writes
	MteAsync,     // Asynchronous tag checking, the cheapest checking mode
	MteStoreOnly, // Tag checking on stores only
	MteFar,       // The faulting address is reported with its tag bits (Linux MTE_FAR)
	Poe,          // Permission overlays, for cheap per thread memory access changes (Linux POE)

	// Security and control flow
	Sb,    // Speculation barrier, for Spectre mitigations
	Ssbs,  // Speculative store bypass control, for Spectre mitigations
	Dit,   // Data independent timing, for constant time crypto
	Bti,   // Branch target identification, for control flow protection
	PAuth, // Pointer authentication, for return address protection (Linux PACA and PACG)
	Gcs,   // Guarded control stack (shadow return stack), which stack switching must handle

	// Persistent memory and counters
	Dpb,  // Cache clean to persistence, for persistent memory (LLVM ccpp, Linux DCPOP)
	Dpb2, // Cache clean to deep persistence, for persistent memory (LLVM ccdp, Linux DCPODP)
	Ecv,  // Self synchronising counter reads (CNTVCTSS), timestamps without a barrier

	Count,
};

struct Arm64MainId {
	std::uint16_t part;
	std::uint8_t implementer;
	std::uint8_t variant;
	// std::uint8_t architecture = 0; // Always the same so no point in keeping
	std::uint8_t revision;
};

#if TRIVIAL_ENABLE_LOGGING
[[nodiscard]] TRIVIAL_COLD constexpr const char* featureName(Arm64Feature feature) noexcept {
	switch (feature) {
		case Arm64Feature::Lse:
			return "FEAT_LSE";
		case Arm64Feature::Lse2:
			return "FEAT_LSE2";
		case Arm64Feature::Lse128:
			return "FEAT_LSE128";
		case Arm64Feature::Lsfe:
			return "FEAT_LSFE";
		case Arm64Feature::Lrcpc:
			return "FEAT_LRCPC";
		case Arm64Feature::Lrcpc2:
			return "FEAT_LRCPC2";
		case Arm64Feature::Lrcpc3:
			return "FEAT_LRCPC3";
		case Arm64Feature::Mops:
			return "FEAT_MOPS";
		case Arm64Feature::Rprfm:
			return "FEAT_RPRFM";
		case Arm64Feature::Wfxt:
			return "FEAT_WFxT";
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
		case Arm64Feature::Rng:
			return "FEAT_RNG";
		case Arm64Feature::Cssc:
			return "FEAT_CSSC";
		case Arm64Feature::FlagM:
			return "FEAT_FlagM";
		case Arm64Feature::FlagM2:
			return "FEAT_FlagM2";
		case Arm64Feature::Jscvt:
			return "FEAT_JSCVT";
		case Arm64Feature::Hbc:
			return "FEAT_HBC";
		case Arm64Feature::Cmpbr:
			return "FEAT_CMPBR";
		case Arm64Feature::Frintts:
			return "FEAT_FRINTTS";
		case Arm64Feature::Fprcvt:
			return "FEAT_FPRCVT";
		case Arm64Feature::Afp:
			return "FEAT_AFP";
		case Arm64Feature::Rpres:
			return "FEAT_RPRES";
		case Arm64Feature::FpSyncExceptions:
			return "FP_SyncExceptions";
		case Arm64Feature::Rdm:
			return "FEAT_RDM";
		case Arm64Feature::DotProd:
			return "FEAT_DotProd";
		case Arm64Feature::Fp16:
			return "FEAT_FP16";
		case Arm64Feature::Fhm:
			return "FEAT_FHM";
		case Arm64Feature::F16f32dot:
			return "FEAT_F16F32DOT";
		case Arm64Feature::F16f32mm:
			return "FEAT_F16F32MM";
		case Arm64Feature::F16mm:
			return "FEAT_F16MM";
		case Arm64Feature::Fcma:
			return "FEAT_FCMA";
		case Arm64Feature::I8mm:
			return "FEAT_I8MM";
		case Arm64Feature::Bf16:
			return "FEAT_BF16";
		case Arm64Feature::Ebf16:
			return "FEAT_EBF16";
		case Arm64Feature::Lut:
			return "FEAT_LUT";
		case Arm64Feature::Faminmax:
			return "FEAT_FAMINMAX";
		case Arm64Feature::Fpmr:
			return "FEAT_FPMR";
		case Arm64Feature::Fp8:
			return "FEAT_FP8";
		case Arm64Feature::Fp8Fma:
			return "FEAT_FP8FMA";
		case Arm64Feature::Fp8Dot4:
			return "FEAT_FP8DOT4";
		case Arm64Feature::Fp8Dot2:
			return "FEAT_FP8DOT2";
		case Arm64Feature::F8f16mm:
			return "FEAT_F8F16MM";
		case Arm64Feature::F8f32mm:
			return "FEAT_F8F32MM";
		case Arm64Feature::Sve:
			return "FEAT_SVE";
		case Arm64Feature::Sve2:
			return "FEAT_SVE2";
		case Arm64Feature::Sve2p1:
			return "FEAT_SVE2p1";
		case Arm64Feature::Sve2p2:
			return "FEAT_SVE2p2";
		case Arm64Feature::Sve2p3:
			return "FEAT_SVE2p3";
		case Arm64Feature::SveI8mm:
			return "HWCAP2_SVEI8MM";
		case Arm64Feature::SveBf16:
			return "HWCAP2_SVEBF16";
		case Arm64Feature::SveEbf16:
			return "HWCAP2_SVE_EBF16";
		case Arm64Feature::SveF32mm:
			return "FEAT_F32MM";
		case Arm64Feature::SveF64mm:
			return "FEAT_F64MM";
		case Arm64Feature::SveF16f32mm:
			return "FEAT_SVE_F16F32MM";
		case Arm64Feature::SveB16b16:
			return "FEAT_SVE_B16B16";
		case Arm64Feature::SveB16mm:
			return "FEAT_SVE_B16MM";
		case Arm64Feature::SveBfscale:
			return "FEAT_SVE_BFSCALE";
		case Arm64Feature::SveAes:
			return "FEAT_SVE_AES";
		case Arm64Feature::SvePmull128:
			return "FEAT_SVE_PMULL128";
		case Arm64Feature::SveAes2:
			return "FEAT_SVE_AES2";
		case Arm64Feature::SveBitperm:
			return "FEAT_SVE_BitPerm";
		case Arm64Feature::SveSha3:
			return "FEAT_SVE_SHA3";
		case Arm64Feature::SveSm4:
			return "FEAT_SVE_SM4";
		case Arm64Feature::SvePerm256:
			return "FEAT_SVE_PERM256";
		case Arm64Feature::SveLut6:
			return "HWCAP3_SVE_LUT6";
		case Arm64Feature::Sme:
			return "FEAT_SME";
		case Arm64Feature::Sme2:
			return "FEAT_SME2";
		case Arm64Feature::Sme2p1:
			return "FEAT_SME2p1";
		case Arm64Feature::Sme2p2:
			return "FEAT_SME2p2";
		case Arm64Feature::Sme2p3:
			return "FEAT_SME2p3";
		case Arm64Feature::SmeF64F64:
			return "FEAT_SME_F64F64";
		case Arm64Feature::SmeI16I64:
			return "FEAT_SME_I16I64";
		case Arm64Feature::SmeF16F16:
			return "FEAT_SME_F16F16";
		case Arm64Feature::SmeB16B16:
			return "FEAT_SME_B16B16";
		case Arm64Feature::SmeF8F16:
			return "FEAT_SME_F8F16";
		case Arm64Feature::SmeF8F32:
			return "FEAT_SME_F8F32";
		case Arm64Feature::SmeLutv2:
			return "FEAT_SME_LUTv2";
		case Arm64Feature::SmeLut6:
			return "HWCAP3_SME_LUT6";
		case Arm64Feature::SmeTmop:
			return "FEAT_SME_TMOP";
		case Arm64Feature::SmeMop4:
			return "FEAT_SME_MOP4";
		case Arm64Feature::SmeFa64:
			return "FEAT_SME_FA64";
		case Arm64Feature::SsveAes:
			return "FEAT_SSVE_AES";
		case Arm64Feature::SsveBitperm:
			return "FEAT_SSVE_BitPerm";
		case Arm64Feature::SsveFexpa:
			return "FEAT_SSVE_FEXPA";
		case Arm64Feature::SsveFp8Fma:
			return "FEAT_SSVE_FP8FMA";
		case Arm64Feature::SsveFp8Dot4:
			return "FEAT_SSVE_FP8DOT4";
		case Arm64Feature::SsveFp8Dot2:
			return "FEAT_SSVE_FP8DOT2";
		case Arm64Feature::SsveF8f16mm:
			return "FEAT_SSVE_F8F16MM";
		case Arm64Feature::SsveF8f32mm:
			return "FEAT_SSVE_F8F32MM";
		case Arm64Feature::Mte:
			return "FEAT_MTE";
		case Arm64Feature::Mte2:
			return "FEAT_MTE2";
		case Arm64Feature::Mte3:
			return "FEAT_MTE3";
		case Arm64Feature::MteAsync:
			return "FEAT_MTE_ASYNC";
		case Arm64Feature::MteStoreOnly:
			return "FEAT_MTE_STORE_ONLY";
		case Arm64Feature::MteFar:
			return "FEAT_MTE_TAGGED_FAR";
		case Arm64Feature::Poe:
			return "FEAT_S1POE";
		case Arm64Feature::Sb:
			return "FEAT_SB";
		case Arm64Feature::Ssbs:
			return "FEAT_SSBS";
		case Arm64Feature::Dit:
			return "FEAT_DIT";
		case Arm64Feature::Bti:
			return "FEAT_BTI";
		case Arm64Feature::PAuth:
			return "FEAT_PAuth";
		case Arm64Feature::Gcs:
			return "FEAT_GCS";
		case Arm64Feature::Dpb:
			return "FEAT_DPB";
		case Arm64Feature::Dpb2:
			return "FEAT_DPB2";
		case Arm64Feature::Ecv:
			return "FEAT_ECV";
		case Arm64Feature::Count:
			break;
	}

	TRIVIAL_UNREACHABLE();
}
#endif // TRIVIAL_ENABLE_LOGGING

namespace detail {

[[nodiscard]] consteval FeatureSet<Arm64Feature> targetFeatures() noexcept {
	constexpr std::array kAppleM1{
	    Arm64Feature::Lse,     Arm64Feature::Lse2, Arm64Feature::Lrcpc,   Arm64Feature::Lrcpc2, Arm64Feature::Rdm,
	    Arm64Feature::DotProd, Arm64Feature::Fp16, Arm64Feature::Fhm,     Arm64Feature::Crc32,  Arm64Feature::Aes,
	    Arm64Feature::Pmull,   Arm64Feature::Sha1, Arm64Feature::Sha256,  Arm64Feature::Sha3,   Arm64Feature::Sha512,
	    Arm64Feature::Jscvt,   Arm64Feature::Fcma, Arm64Feature::Frintts, Arm64Feature::FlagM,  Arm64Feature::FlagM2,
	    Arm64Feature::Sb,      Arm64Feature::Dit,  Arm64Feature::Dpb,     Arm64Feature::Dpb2,   Arm64Feature::PAuth,
	};
	FeatureSet<Arm64Feature> features;

	if constexpr (TRIVIAL_CPU_ARM64_APPLE_M1) {
		for (const Arm64Feature kFeature : kAppleM1) {
			features.insert(kFeature);
		}
	}

	return features;
}

[[nodiscard]] consteval FeatureSet<Arm64Feature> compilerTargetFeatures() noexcept {
	FeatureSet<Arm64Feature> features;
#ifdef __ARM_FEATURE_CRC32
	features.insert(Arm64Feature::Crc32);
#endif
#ifdef __ARM_FEATURE_AES
	features.insert(Arm64Feature::Aes);
	features.insert(Arm64Feature::Pmull);
#endif
#ifdef __ARM_FEATURE_SHA2
	features.insert(Arm64Feature::Sha1);
	features.insert(Arm64Feature::Sha256);
#endif
#ifdef __ARM_FEATURE_SHA3
	features.insert(Arm64Feature::Sha3);
#endif
#ifdef __ARM_FEATURE_SHA512
	features.insert(Arm64Feature::Sha512);
#endif
#ifdef __ARM_FEATURE_SM3
	features.insert(Arm64Feature::Sm3);
#endif
#ifdef __ARM_FEATURE_SM4
	features.insert(Arm64Feature::Sm4);
#endif
#ifdef __ARM_FEATURE_RNG
	features.insert(Arm64Feature::Rng);
#endif
#ifdef __ARM_FEATURE_ATOMICS
	features.insert(Arm64Feature::Lse);
#endif
#ifdef __ARM_FEATURE_QRDMX
	features.insert(Arm64Feature::Rdm);
#endif
#ifdef __ARM_FEATURE_RCPC
	features.insert(Arm64Feature::Lrcpc);
#endif
#ifdef __ARM_FEATURE_DOTPROD
	features.insert(Arm64Feature::DotProd);
#endif
#ifdef __ARM_FEATURE_FP16_SCALAR_ARITHMETIC
	features.insert(Arm64Feature::Fp16);
#endif
#ifdef __ARM_FEATURE_FP16_FML
	features.insert(Arm64Feature::Fhm);
#endif
#ifdef __ARM_FEATURE_F16F32DOT
	features.insert(Arm64Feature::F16f32dot);
#endif
#ifdef __ARM_FEATURE_F16F32MM
	features.insert(Arm64Feature::F16f32mm);
#endif
#ifdef __ARM_FEATURE_F16MM
	features.insert(Arm64Feature::F16mm);
#endif
#ifdef __ARM_FEATURE_JCVT
	features.insert(Arm64Feature::Jscvt);
#endif
#ifdef __ARM_FEATURE_COMPLEX
	features.insert(Arm64Feature::Fcma);
#endif
#ifdef __ARM_FEATURE_FRINT
	features.insert(Arm64Feature::Frintts);
#endif
#ifdef __ARM_FEATURE_FPRCVT
	features.insert(Arm64Feature::Fprcvt);
#endif
#ifdef __ARM_FEATURE_PAUTH
	features.insert(Arm64Feature::PAuth);
#endif
#ifdef __ARM_FEATURE_MATMUL_INT8
	features.insert(Arm64Feature::I8mm);
#endif
#ifdef __ARM_FEATURE_BF16_VECTOR_ARITHMETIC
	features.insert(Arm64Feature::Bf16);
#endif
#ifdef __ARM_FEATURE_FP8
	features.insert(Arm64Feature::Fp8);
#endif
#ifdef __ARM_FEATURE_FP8FMA
	features.insert(Arm64Feature::Fp8Fma);
#endif
#ifdef __ARM_FEATURE_FP8DOT4
	features.insert(Arm64Feature::Fp8Dot4);
#endif
#ifdef __ARM_FEATURE_FP8DOT2
	features.insert(Arm64Feature::Fp8Dot2);
#endif
#ifdef __ARM_FEATURE_F8F16MM
	features.insert(Arm64Feature::F8f16mm);
#endif
#ifdef __ARM_FEATURE_F8F32MM
	features.insert(Arm64Feature::F8f32mm);
#endif
#ifdef __ARM_FEATURE_MEMORY_TAGGING
	features.insert(Arm64Feature::Mte);
#endif
#ifdef __ARM_FEATURE_SVE
	features.insert(Arm64Feature::Sve);
#endif
#ifdef __ARM_FEATURE_SVE2
	features.insert(Arm64Feature::Sve2);
#endif
#ifdef __ARM_FEATURE_SVE_MATMUL_INT8
	features.insert(Arm64Feature::SveI8mm);
#endif
#ifdef __ARM_FEATURE_SVE_BF16
	features.insert(Arm64Feature::SveBf16);
#endif
#ifdef __ARM_FEATURE_SVE_MATMUL_FP32
	features.insert(Arm64Feature::SveF32mm);
#endif
#ifdef __ARM_FEATURE_SVE_MATMUL_FP64
	features.insert(Arm64Feature::SveF64mm);
#endif
#ifdef __ARM_FEATURE_SVE2_AES
	features.insert(Arm64Feature::SveAes);
	features.insert(Arm64Feature::SvePmull128);
#endif
#ifdef __ARM_FEATURE_SVE2_BITPERM
	features.insert(Arm64Feature::SveBitperm);
#endif
#ifdef __ARM_FEATURE_SVE_B16B16
	features.insert(Arm64Feature::SveB16b16);
#endif
#ifdef __ARM_FEATURE_SVE2p1
	features.insert(Arm64Feature::Sve2p1);
#endif
#ifdef __ARM_FEATURE_SVE2p2
	features.insert(Arm64Feature::Sve2p2);
#endif
#ifdef __ARM_FEATURE_SVE2p3
	features.insert(Arm64Feature::Sve2p3);
#endif
#ifdef __ARM_FEATURE_SVE_AES2
	features.insert(Arm64Feature::SveAes2);
#endif
#ifdef __ARM_FEATURE_SVE_BFSCALE
	features.insert(Arm64Feature::SveBfscale);
#endif
#ifdef __ARM_FEATURE_SVE_B16MM
	features.insert(Arm64Feature::SveB16mm);
#endif
#ifdef __ARM_FEATURE_SVE_F16F32MM
	features.insert(Arm64Feature::SveF16f32mm);
#endif
#ifdef __ARM_FEATURE_SME
	features.insert(Arm64Feature::Sme);
#endif
#ifdef __ARM_FEATURE_SME2
	features.insert(Arm64Feature::Sme2);
#endif
#ifdef __ARM_FEATURE_SME2p1
	features.insert(Arm64Feature::Sme2p1);
#endif
#ifdef __ARM_FEATURE_SME2p2
	features.insert(Arm64Feature::Sme2p2);
#endif
#ifdef __ARM_FEATURE_SME2p3
	features.insert(Arm64Feature::Sme2p3);
#endif
#ifdef __ARM_FEATURE_SME_F16F16
	features.insert(Arm64Feature::SmeF16F16);
#endif
#ifdef __ARM_FEATURE_SME_B16B16
	features.insert(Arm64Feature::SmeB16B16);
#endif
#ifdef __ARM_FEATURE_SME_F8F16
	features.insert(Arm64Feature::SmeF8F16);
#endif
#ifdef __ARM_FEATURE_SME_F8F32
	features.insert(Arm64Feature::SmeF8F32);
#endif
#ifdef __ARM_FEATURE_SSVE_FP8FMA
	features.insert(Arm64Feature::SsveFp8Fma);
#endif
#ifdef __ARM_FEATURE_SSVE_FP8DOT4
	features.insert(Arm64Feature::SsveFp8Dot4);
#endif
#ifdef __ARM_FEATURE_SSVE_FP8DOT2
	features.insert(Arm64Feature::SsveFp8Dot2);
#endif
#ifdef __ARM_FEATURE_SSVE_AES
	features.insert(Arm64Feature::SsveAes);
#endif
#ifdef __ARM_FEATURE_CSSC
	features.insert(Arm64Feature::Cssc);
#endif
#ifdef __ARM_FEATURE_MOPS
	features.insert(Arm64Feature::Mops);
#endif
#ifdef __ARM_FEATURE_GCS
	features.insert(Arm64Feature::Gcs);
#endif

	return features;
}

static_assert(
    targetFeatures().containsAll(compilerTargetFeatures()),
    "The compiler targets CPU features beyond the build's CPU version, which the floor check would not cover");

} // namespace detail

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr bool isTargetFeature(Arm64Feature feature) noexcept {
	return detail::targetFeatures().contains(feature);
}

} // namespace trivial::cpu

#endif // TRIVIAL_ARCH_ARM64

#endif // TRIVIAL_CORE_CPU_CPU_FEATURES_ARM64_H

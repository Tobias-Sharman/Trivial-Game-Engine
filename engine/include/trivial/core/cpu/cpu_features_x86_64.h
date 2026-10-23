#ifndef TRIVIAL_CORE_CPU_CPU_FEATURES_X86_64_H
#define TRIVIAL_CORE_CPU_CPU_FEATURES_X86_64_H

#include <trivial/core/platform.h>

#if TRIVIAL_ARCH_X86_64
#include <cstdint>

#include <trivial/core/assert.h>
#include <trivial/core/compiler.h>
#include <trivial/core/config.h>
#include <trivial/core/cpu/cpu_feature_set.h>

#if (TRIVIAL_CPU_X86_64_LEVEL != 2) && (TRIVIAL_CPU_X86_64_LEVEL != 3)
#error "TRIVIAL_CPU_X86_64_LEVEL must be 2 or 3"
#endif

#if TRIVIAL_COMPILER_MSVC || (defined(__SSE4_2__) && defined(__POPCNT__))
#define TRIVIAL_CPU_X86_64_TARGET_HAS_V2 1
#else
#define TRIVIAL_CPU_X86_64_TARGET_HAS_V2 0
#endif

#if defined(__AVX2__) && (TRIVIAL_COMPILER_MSVC || (defined(__BMI2__) && defined(__FMA__)))
#define TRIVIAL_CPU_X86_64_TARGET_HAS_V3 1
#else
#define TRIVIAL_CPU_X86_64_TARGET_HAS_V3 0
#endif

#if !TRIVIAL_CPU_X86_64_TARGET_HAS_V2
#error "Compiler target is below x86-64 v2, the flags from trivial_apply_cpu_level() are missing"
#endif

#if (TRIVIAL_CPU_X86_64_LEVEL >= 3) && !TRIVIAL_CPU_X86_64_TARGET_HAS_V3
#error "Compiler target is below x86-64 v3, the flags from trivial_apply_cpu_level() are missing"
#endif

#undef TRIVIAL_CPU_X86_64_TARGET_HAS_V2
#undef TRIVIAL_CPU_X86_64_TARGET_HAS_V3

namespace trivial::cpu {

// NOLINTNEXTLINE(readability-identifier-naming)
enum class X86_64Feature : std::uint8_t {
	Sse3,              // Horizontal float adds, for reductions
	Pclmulqdq,         // Carry-less multiply, for fast CRC and AES-GCM (LLVM pclmul)
	Ssse3,             // Byte shuffles (PSHUFB), for table lookups and swizzles
	Fma,               // Fused multiply-add, faster and more accurate a * b + c
	Cmpxchg16b,        // 16-byte compare and swap, for lock-free pointer and tag pairs (LLVM cx16)
	Sse41,             // Blends, rounding and dot products, for general SIMD maths
	Sse42,             // String comparison and CRC32, for text scanning and hashing
	Movbe,             // Byte-swapping loads and stores, for endian conversion
	Popcnt,            // Population count, for bit sets and masks
	Aesni,             // AES rounds, for encryption and fast hashing (LLVM aes)
	Xsave,             // Extended register save and restore, which the OS needs for AVX
	Osxsave,           // OS has enabled XSAVE, needed before AVX can be trusted
	Avx,               // 256-bit float vectors, for wider SIMD maths
	F16c,              // Half float conversion, for compact vertex and texture data
	Rdrand,            // Hardware random numbers, for seeding (LLVM rdrnd)
	HypervisorPresent, // Running inside a virtual machine, for diagnostics

	Bmi1,              // Trailing zero count and bit clearing, for iterating bit sets (LLVM bmi)
	Avx2,              // 256-bit integer vectors and gathers, for wider integer SIMD
	Bmi2,              // Bit deposit and extract (PDEP, PEXT), for Morton codes and packing
	EnhancedRepMovsb,  // Fast REP MOVSB, for large memory copies (LLVM ermsb)
	Avx512F,           // 512-bit vectors, the base of all AVX-512
	Avx512Dq,          // AVX-512 64-bit integer maths and conversions
	Rdseed,            // Hardware entropy, for seeding random generators
	Adx,               // Add with carry chains, for big integer maths
	Avx512Ifma,        // 52-bit integer multiply-add, for big integer and crypto maths
	Clflushopt,        // Cache line flush, for persistent memory and device writes
	Clwb,              // Cache line write back without eviction, for persistent memory
	Avx512Cd,          // Conflict detection, for vectorised scatters with duplicates
	Sha,               // SHA-1 and SHA-256 hashing
	Avx512Bw,          // AVX-512 on bytes and words, for 8 and 16-bit SIMD
	Avx512Vl,          // AVX-512 instructions at 128 and 256-bit widths
	Avx512Vbmi,        // Byte permutes across a vector, for table lookups
	Waitpkg,           // Low power user mode waits, for spin loops
	Avx512Vbmi2,       // Compress, expand and funnel shifts, for stream compaction
	Gfni,              // Galois field maths, for bit matrix transforms and erasure codes
	Vaes,              // AES on whole vectors, for bulk encryption
	Vpclmulqdq,        // Carry-less multiply on whole vectors, for bulk CRC
	Avx512Vnni,        // AVX-512 int8 and int16 dot products, for inference
	Avx512Bitalg,      // Byte and word population count on vectors
	Avx512Vpopcntdq,   // 32 and 64-bit population count on vectors
	FastShortRepMovsb, // Fast short REP MOVSB, for small memory copies (LLVM fsrm)
	Serialize,         // Instruction serialisation, for precise timing
	Hybrid,            // Mix of performance and efficiency cores, for scheduling
	AmxBf16,           // AMX bfloat16 matrix multiply, for machine learning
	Avx512Fp16,        // AVX-512 half precision maths
	AmxTile,           // AMX tile registers, the base of all AMX
	AmxInt8,           // AMX int8 matrix multiply, for machine learning

	Sha512,       // SHA-512 hashing
	Sm3,          // SM3 hashing, a Chinese standard
	Sm4,          // SM4 encryption, a Chinese standard
	AvxVnni,      // Int8 and int16 dot products without AVX-512, for inference
	Avx512Bf16,   // AVX-512 bfloat16 dot products, for machine learning
	AvxIfma,      // 52-bit integer multiply-add without AVX-512
	AvxVnniInt8,  // Signed and unsigned int8 dot products without AVX-512
	AvxNeConvert, // Bfloat16 and half conversions without AVX-512
	AvxVnniInt16, // Int16 dot products without AVX-512
	Avx10,        // Converged AVX-512 successor, avx10Version says what it contains (LLVM avx10.1 and later)
	ApxF,         // 32 general registers and three operand forms, for less spilling (LLVM and GCC apxf)

	LahfSahf,     // LAHF and SAHF in 64-bit mode, for saving and restoring flags (LLVM sahf)
	Lzcnt,        // Leading zero count, for log2 and bit scanning
	Prefetchw,    // Prefetch for writing, for data about to be modified (LLVM prfchw)
	Rdtscp,       // Timestamp read that waits for earlier instructions, for timing
	InvariantTsc, // Timestamp counter runs at a constant rate, usable as a clock

	Count,
};

struct X86_64Info { // NOLINT(readability-identifier-naming)
	std::uint16_t family = 0;
	std::uint8_t model = 0;
	std::uint8_t stepping = 0;
	std::uint8_t avx10Version = 0;
};

[[nodiscard]] TRIVIAL_COLD constexpr const char* featureName(X86_64Feature feature) noexcept {
	switch (feature) {
		case X86_64Feature::Sse3:
			return "SSE3";
		case X86_64Feature::Pclmulqdq:
			return "PCLMULQDQ";
		case X86_64Feature::Ssse3:
			return "SSSE3";
		case X86_64Feature::Fma:
			return "FMA";
		case X86_64Feature::Cmpxchg16b:
			return "CMPXCHG16B";
		case X86_64Feature::Sse41:
			return "SSE4_1";
		case X86_64Feature::Sse42:
			return "SSE4_2";
		case X86_64Feature::Movbe:
			return "MOVBE";
		case X86_64Feature::Popcnt:
			return "POPCNT";
		case X86_64Feature::Aesni:
			return "AESNI";
		case X86_64Feature::Xsave:
			return "XSAVE";
		case X86_64Feature::Osxsave:
			return "OSXSAVE";
		case X86_64Feature::Avx:
			return "AVX";
		case X86_64Feature::F16c:
			return "F16C";
		case X86_64Feature::Rdrand:
			return "RDRAND";
		case X86_64Feature::HypervisorPresent:
			return "Hypervisor present";
		case X86_64Feature::Bmi1:
			return "BMI1";
		case X86_64Feature::Avx2:
			return "AVX2";
		case X86_64Feature::Bmi2:
			return "BMI2";
		case X86_64Feature::EnhancedRepMovsb:
			return "Enhanced REP MOVSB/STOSB";
		case X86_64Feature::Avx512F:
			return "AVX512F";
		case X86_64Feature::Avx512Dq:
			return "AVX512DQ";
		case X86_64Feature::Rdseed:
			return "RDSEED";
		case X86_64Feature::Adx:
			return "ADX";
		case X86_64Feature::Avx512Ifma:
			return "AVX512_IFMA";
		case X86_64Feature::Clflushopt:
			return "CLFLUSHOPT";
		case X86_64Feature::Clwb:
			return "CLWB";
		case X86_64Feature::Avx512Cd:
			return "AVX512CD";
		case X86_64Feature::Sha:
			return "SHA";
		case X86_64Feature::Avx512Bw:
			return "AVX512BW";
		case X86_64Feature::Avx512Vl:
			return "AVX512VL";
		case X86_64Feature::Avx512Vbmi:
			return "AVX512_VBMI";
		case X86_64Feature::Waitpkg:
			return "WAITPKG";
		case X86_64Feature::Avx512Vbmi2:
			return "AVX512_VBMI2";
		case X86_64Feature::Gfni:
			return "GFNI";
		case X86_64Feature::Vaes:
			return "VAES";
		case X86_64Feature::Vpclmulqdq:
			return "VPCLMULQDQ";
		case X86_64Feature::Avx512Vnni:
			return "AVX512_VNNI";
		case X86_64Feature::Avx512Bitalg:
			return "AVX512_BITALG";
		case X86_64Feature::Avx512Vpopcntdq:
			return "AVX512_VPOPCNTDQ";
		case X86_64Feature::FastShortRepMovsb:
			return "Fast Short REP MOVSB";
		case X86_64Feature::Serialize:
			return "SERIALIZE";
		case X86_64Feature::Hybrid:
			return "Hybrid";
		case X86_64Feature::AmxBf16:
			return "AMX-BF16";
		case X86_64Feature::Avx512Fp16:
			return "AVX512_FP16";
		case X86_64Feature::AmxTile:
			return "AMX-TILE";
		case X86_64Feature::AmxInt8:
			return "AMX-INT8";
		case X86_64Feature::Sha512:
			return "SHA512";
		case X86_64Feature::Sm3:
			return "SM3";
		case X86_64Feature::Sm4:
			return "SM4";
		case X86_64Feature::AvxVnni:
			return "AVX-VNNI";
		case X86_64Feature::Avx512Bf16:
			return "AVX512_BF16";
		case X86_64Feature::AvxIfma:
			return "AVX-IFMA";
		case X86_64Feature::AvxVnniInt8:
			return "AVX-VNNI-INT8";
		case X86_64Feature::AvxNeConvert:
			return "AVX-NE-CONVERT";
		case X86_64Feature::AvxVnniInt16:
			return "AVX-VNNI-INT16";
		case X86_64Feature::Avx10:
			return "AVX10";
		case X86_64Feature::ApxF:
			return "APX_F";
		case X86_64Feature::LahfSahf:
			return "LAHF/SAHF";
		case X86_64Feature::Lzcnt:
			return "LZCNT";
		case X86_64Feature::Prefetchw:
			return "PREFETCHW";
		case X86_64Feature::Rdtscp:
			return "RDTSCP";
		case X86_64Feature::InvariantTsc:
			return "Invariant TSC";
		case X86_64Feature::Count:
			break;
	}

	TRIVIAL_UNREACHABLE();
}

namespace detail {

// NOLINTNEXTLINE(readability-identifier-naming)
[[nodiscard]] consteval FeatureSet<X86_64Feature> guaranteedX86_64Features() noexcept {
	FeatureSet<X86_64Feature> features;

	features.insert(X86_64Feature::Sse3);
	features.insert(X86_64Feature::Ssse3);
	features.insert(X86_64Feature::Sse41);
	features.insert(X86_64Feature::Sse42);
	features.insert(X86_64Feature::Popcnt);
	features.insert(X86_64Feature::Cmpxchg16b);
	features.insert(X86_64Feature::LahfSahf);

	if constexpr (TRIVIAL_CPU_X86_64_LEVEL >= 3) {
		features.insert(X86_64Feature::Avx);
		features.insert(X86_64Feature::Avx2);
		features.insert(X86_64Feature::Bmi1);
		features.insert(X86_64Feature::Bmi2);
		features.insert(X86_64Feature::Fma);
		features.insert(X86_64Feature::F16c);
		features.insert(X86_64Feature::Lzcnt);
		features.insert(X86_64Feature::Movbe);
		features.insert(X86_64Feature::Osxsave);
	}

	return features;
}

} // namespace detail

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr bool isGuaranteed(X86_64Feature feature) noexcept {
	return detail::guaranteedX86_64Features().contains(feature);
}

} // namespace trivial::cpu

#endif // TRIVIAL_ARCH_X86_64

#endif // TRIVIAL_CORE_CPU_CPU_FEATURES_X86_64_H

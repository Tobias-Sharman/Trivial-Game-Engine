#ifndef TRIVIAL_CORE_CPU_CPU_FEATURES_X86_64_H
#define TRIVIAL_CORE_CPU_CPU_FEATURES_X86_64_H

#include <trivial/core/platform.h>

#if TRIVIAL_ARCH_X86_64
#include <array>
#include <cstdint>

#include <trivial/core/compiler.h>
#include <trivial/core/config.h>
#include <trivial/core/cpu/cpu_feature_set.h>

#if TRIVIAL_ENABLE_LOGGING
#include <trivial/core/assert.h>
#endif // TRIVIAL_ENABLE_LOGGING

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
#error "Compiler target is below x86-64 v2, the flags from trivial_apply_cpu_version() are missing"
#endif

#if (TRIVIAL_CPU_X86_64_VERSION >= 3) && !TRIVIAL_CPU_X86_64_TARGET_HAS_V3
#error "Compiler target is below x86-64 v3, the flags from trivial_apply_cpu_version() are missing"
#endif

#undef TRIVIAL_CPU_X86_64_TARGET_HAS_V2
#undef TRIVIAL_CPU_X86_64_TARGET_HAS_V3

namespace trivial::cpu {

// NOLINTNEXTLINE(readability-identifier-naming)
enum class X86_64Feature : std::uint8_t {
	// Atomics, memory ordering, copies and waits
	Cmpxchg16b,             // 16-byte compare and swap, for lock-free pointer-tag pairs (LLVM cx16, CPUID 1 ECX[13])
	Cmpccxadd,              // Compare and add atomics, for lock-free algorithms (CPUID 7.1 EAX[7])
	RaoInt,                 // Remote atomic operations, for heavily contended counters (LLVM raoint, CPUID 7.1 EAX[3])
	Rtm,                    // Restricted transactional memory, mostly turned off by microcode (CPUID 7.0 EBX[11])
	Movdiri,                // Direct stores, for device writes (CPUID 7.0 ECX[27])
	Movdir64b,              // 64-byte direct stores, for device writes (CPUID 7.0 ECX[28])
	Serialize,              // Instruction serialisation, for precise timing (CPUID 7.0 EDX[14])
	EnhancedRepMovsb,       // Fast REP MOVSB, for large memory copies (LLVM ermsb, CPUID 7.0 EBX[9])
	FastShortRepMovsb,      // Fast short REP MOVSB, for small memory copies (LLVM fsrm, CPUID 7.0 EDX[4])
	FastZeroLengthRepMovsb, // Fast zero length REP MOVSB, for memory copies that may be empty (CPUID 7.1 EAX[10])
	FastShortRepStosb,      // Fast short REP STOSB, for small memory sets (CPUID 7.1 EAX[11])
	FastShortRepCmpsbScasb, // Fast short REP CMPSB and SCASB, for small compares and scans (CPUID 7.1 EAX[12])
	Waitpkg,                // Low power user mode waits, for spin loops (CPUID 7.0 ECX[5])
	Monitorx,               // AMD user mode monitor and wait, for spin loops (LLVM mwaitx, CPUID 8000_0001 ECX[29])

	// Cache control and prefetches
	Clflushopt, // Cache line flush, for persistent memory and device writes (CPUID 7.0 EBX[23])
	Clwb,       // Cache line write back without eviction, for persistent memory (CPUID 7.0 EBX[24])
	Cldemote,   // Cache line demote to share with other cores, Xeon only, a no-op elsewhere (CPUID 7.0 ECX[25])
	Clzero,     // AMD cache line zeroing (CPUID 8000_0008 EBX[0])
	Prefetchw,  // Prefetch for writing, for data about to be modified (LLVM prfchw, CPUID 8000_0001 ECX[8])
	Prefetchi,  // Code prefetches, for hot code paths (LLVM prefetchi, CPUID 7.1 EDX[14] or 8000_0021 EAX[20])
	Movrs,      // Read shared loads, for data read by many cores (CPUID 7.1 EAX[31])

	// Checksums, hashing, encryption and random numbers
	Pclmulqdq,  // Carry-less multiply, for fast CRC and AES-GCM (LLVM pclmul, CPUID 1 ECX[1])
	Aesni,      // AES rounds, for encryption and fast hashing (LLVM aes, CPUID 1 ECX[25])
	Sha,        // SHA-1 and SHA-256 hashing (CPUID 7.0 EBX[29])
	Sha512,     // SHA-512 hashing (CPUID 7.1 EAX[0])
	Sm3,        // SM3 hashing, a Chinese standard (CPUID 7.1 EAX[1])
	Sm4,        // SM4 encryption, a Chinese standard (CPUID 7.1 EAX[2])
	Gfni,       // Galois field maths, for bit matrix transforms and erasure codes (CPUID 7.0 ECX[8])
	Vaes,       // AES on whole vectors, for bulk encryption (CPUID 7.0 ECX[9])
	Vpclmulqdq, // Carry-less multiply on whole vectors, for bulk CRC (CPUID 7.0 ECX[10])
	Rdrand,     // Hardware random numbers, for seeding (LLVM rdrnd, CPUID 1 ECX[30])
	Rdseed,     // Hardware entropy, for seeding random generators (CPUID 7.0 EBX[18])

	// General purpose and bit manipulation
	Popcnt,   // Population count, for bit sets and masks (CPUID 1 ECX[23])
	Lzcnt,    // Leading zero count, for log2 and bit scanning (CPUID 8000_0001 ECX[5])
	Bmi1,     // Trailing zero count and bit clearing, for iterating bit sets (LLVM bmi, CPUID 7.0 EBX[3])
	Bmi2,     // Bit deposit and extract (PDEP, PEXT), for Morton codes and packing (CPUID 7.0 EBX[8])
	Tbm,      // AMD trailing bit manipulation, Bulldozer family only (CPUID 8000_0001 ECX[21])
	Adx,      // Add with carry chains, for big integer maths (CPUID 7.0 EBX[19])
	Movbe,    // Byte-swapping loads and stores, for endian conversion (CPUID 1 ECX[22])
	LahfSahf, // LAHF and SAHF in 64-bit mode, for saving and restoring flags (LLVM sahf, CPUID 8000_0001 ECX[0])
	Rdpid,    // Read the current processor ID, without the timestamp read of RDTSCP (CPUID 7.0 ECX[22])

	// SSE
	Sse3,  // Horizontal float adds, for reductions (CPUID 1 ECX[0])
	Ssse3, // Byte shuffles (PSHUFB), for table lookups and swizzles (CPUID 1 ECX[9])
	Sse41, // Blends, rounding and dot products, for general SIMD maths (CPUID 1 ECX[19])
	Sse42, // String comparison and CRC32, for text scanning and hashing (CPUID 1 ECX[20])
	Sse4a, // AMD SSE4A extract, insert and scalar streaming stores (CPUID 8000_0001 ECX[6])

	// AVX and AVX2, the 256-bit vector extensions
	Avx,          // 256-bit float vectors, for wider SIMD maths (CPUID 1 ECX[28])
	Avx2,         // 256-bit integer vectors and gathers, for wider integer SIMD (CPUID 7.0 EBX[5])
	Fma,          // Fused multiply-add, faster and more accurate a * b + c (CPUID 1 ECX[12])
	F16c,         // Half float conversion, for compact vertex and texture data (CPUID 1 ECX[29])
	AvxVnni,      // Int8 and int16 dot products without AVX-512, for inference (CPUID 7.1 EAX[4])
	AvxVnniInt8,  // Signed and unsigned int8 dot products without AVX-512 (CPUID 7.1 EDX[4])
	AvxVnniInt16, // Int16 dot products without AVX-512 (CPUID 7.1 EDX[10])
	AvxIfma,      // 52-bit integer multiply-add without AVX-512 (CPUID 7.1 EAX[23])
	AvxNeConvert, // Bfloat16 and half conversions without AVX-512 (CPUID 7.1 EDX[5])
	Xop,          // AMD extended vector operations, Bulldozer family only (CPUID 8000_0001 ECX[11])
	Fma4,         // AMD four operand fused multiply-add, Bulldozer family only (CPUID 8000_0001 ECX[16])

	// AVX-512 and AVX10
	Avx512F,            // 512-bit vectors, the base of all AVX-512 (CPUID 7.0 EBX[16])
	Avx512Dq,           // AVX-512 64-bit integer maths and conversions (CPUID 7.0 EBX[17])
	Avx512Cd,           // Conflict detection, for vectorised scatters with duplicates (CPUID 7.0 EBX[28])
	Avx512Bw,           // AVX-512 on bytes and words, for 8 and 16-bit SIMD (CPUID 7.0 EBX[30])
	Avx512Vl,           // AVX-512 instructions at 128 and 256-bit widths (CPUID 7.0 EBX[31])
	Avx512Ifma,         // 52-bit integer multiply-add, for big integer and crypto maths (CPUID 7.0 EBX[21])
	Avx512Vbmi,         // Byte permutes across a vector, for table lookups (CPUID 7.0 ECX[1])
	Avx512Vbmi2,        // Compress, expand and funnel shifts, for stream compaction (CPUID 7.0 ECX[6])
	Avx512Vnni,         // AVX-512 int8 and int16 dot products, for inference (CPUID 7.0 ECX[11])
	Avx512Bitalg,       // Byte and word population count on vectors (CPUID 7.0 ECX[12])
	Avx512Vpopcntdq,    // 32 and 64-bit population count on vectors (CPUID 7.0 ECX[14])
	Avx512Vp2intersect, // AVX-512 intersection masks of dword and qword vectors (CPUID 7.0 EDX[8])
	Avx512Fp16,         // AVX-512 half precision maths (CPUID 7.0 EDX[23])
	Avx512Bf16,         // AVX-512 bfloat16 dot products, for machine learning (CPUID 7.1 EAX[5])
	Avx10V1,            // AVX10 version 1, AVX-512 at every vector length (LLVM avx10.1, CPUID 24 EBX version >= 1)
	Avx10V2,            // AVX10 version 2, AI, media and conversion additions (LLVM avx10.2, CPUID 24 EBX version >= 2)

	// APX
	ApxF, // 32 general registers and three operand forms, for less spilling (LLVM and GCC apxf, CPUID 7.1 EDX[21])

	// Extended register state saving
	Xsave,    // Extended register save and restore, which the OS needs for AVX (CPUID 1 ECX[26])
	Osxsave,  // OS has enabled XSAVE, needed before AVX can be trusted (CPUID 1 ECX[27])
	Xsaveopt, // Extended state save that skips unmodified state (CPUID 0D.1 EAX[0])
	Xsavec,   // Compacted extended state save, for smaller save areas (CPUID 0D.1 EAX[1])

	// Memory protection
	Pku, // Protection keys for user pages, reported only once the OS has enabled them (OSPKE, CPUID 7.0 ECX[4])

	// Timestamps
	Rdtscp,       // Timestamp read that waits for earlier instructions, for timing (CPUID 8000_0001 EDX[27])
	InvariantTsc, // Timestamp counter runs at a constant rate, usable as a clock (CPUID 8000_0007 EDX[8])

	// Information about the machine
	HypervisorPresent, // Running inside a virtual machine, for diagnostics (CPUID 1 ECX[31])
	Hybrid,            // Mix of performance and efficiency cores, for scheduling (CPUID 7.0 EDX[15])

	Count,
};

struct X86_64Info { // NOLINT(readability-identifier-naming)
	std::uint16_t family = 0;
	std::uint8_t model = 0;
	std::uint8_t stepping = 0;
	std::uint8_t avx10Version = 0;
};

#if TRIVIAL_ENABLE_LOGGING
[[nodiscard]] TRIVIAL_COLD constexpr const char* featureName(X86_64Feature feature) noexcept {
	switch (feature) {
		case X86_64Feature::Cmpxchg16b:
			return "CMPXCHG16B";
		case X86_64Feature::Cmpccxadd:
			return "CMPCCXADD";
		case X86_64Feature::RaoInt:
			return "RAO-INT";
		case X86_64Feature::Rtm:
			return "RTM";
		case X86_64Feature::Movdiri:
			return "MOVDIRI";
		case X86_64Feature::Movdir64b:
			return "MOVDIR64B";
		case X86_64Feature::Serialize:
			return "SERIALIZE";
		case X86_64Feature::EnhancedRepMovsb:
			return "Enhanced REP MOVSB/STOSB";
		case X86_64Feature::FastShortRepMovsb:
			return "Fast Short REP MOVSB";
		case X86_64Feature::FastZeroLengthRepMovsb:
			return "Fast Zero-Length REP MOVSB";
		case X86_64Feature::FastShortRepStosb:
			return "Fast Short REP STOSB";
		case X86_64Feature::FastShortRepCmpsbScasb:
			return "Fast Short REP CMPSB/SCASB";
		case X86_64Feature::Waitpkg:
			return "WAITPKG";
		case X86_64Feature::Monitorx:
			return "MONITORX";
		case X86_64Feature::Clflushopt:
			return "CLFLUSHOPT";
		case X86_64Feature::Clwb:
			return "CLWB";
		case X86_64Feature::Cldemote:
			return "CLDEMOTE";
		case X86_64Feature::Clzero:
			return "CLZERO";
		case X86_64Feature::Prefetchw:
			return "PREFETCHW";
		case X86_64Feature::Prefetchi:
			return "PREFETCHI";
		case X86_64Feature::Movrs:
			return "MOVRS";
		case X86_64Feature::Pclmulqdq:
			return "PCLMULQDQ";
		case X86_64Feature::Aesni:
			return "AESNI";
		case X86_64Feature::Sha:
			return "SHA";
		case X86_64Feature::Sha512:
			return "SHA512";
		case X86_64Feature::Sm3:
			return "SM3";
		case X86_64Feature::Sm4:
			return "SM4";
		case X86_64Feature::Gfni:
			return "GFNI";
		case X86_64Feature::Vaes:
			return "VAES";
		case X86_64Feature::Vpclmulqdq:
			return "VPCLMULQDQ";
		case X86_64Feature::Rdrand:
			return "RDRAND";
		case X86_64Feature::Rdseed:
			return "RDSEED";
		case X86_64Feature::Popcnt:
			return "POPCNT";
		case X86_64Feature::Lzcnt:
			return "LZCNT";
		case X86_64Feature::Bmi1:
			return "BMI1";
		case X86_64Feature::Bmi2:
			return "BMI2";
		case X86_64Feature::Tbm:
			return "TBM";
		case X86_64Feature::Adx:
			return "ADX";
		case X86_64Feature::Movbe:
			return "MOVBE";
		case X86_64Feature::LahfSahf:
			return "LAHF/SAHF";
		case X86_64Feature::Rdpid:
			return "RDPID";
		case X86_64Feature::Sse3:
			return "SSE3";
		case X86_64Feature::Ssse3:
			return "SSSE3";
		case X86_64Feature::Sse41:
			return "SSE4_1";
		case X86_64Feature::Sse42:
			return "SSE4_2";
		case X86_64Feature::Sse4a:
			return "SSE4A";
		case X86_64Feature::Avx:
			return "AVX";
		case X86_64Feature::Avx2:
			return "AVX2";
		case X86_64Feature::Fma:
			return "FMA";
		case X86_64Feature::F16c:
			return "F16C";
		case X86_64Feature::AvxVnni:
			return "AVX-VNNI";
		case X86_64Feature::AvxVnniInt8:
			return "AVX-VNNI-INT8";
		case X86_64Feature::AvxVnniInt16:
			return "AVX-VNNI-INT16";
		case X86_64Feature::AvxIfma:
			return "AVX-IFMA";
		case X86_64Feature::AvxNeConvert:
			return "AVX-NE-CONVERT";
		case X86_64Feature::Xop:
			return "XOP";
		case X86_64Feature::Fma4:
			return "FMA4";
		case X86_64Feature::Avx512F:
			return "AVX512F";
		case X86_64Feature::Avx512Dq:
			return "AVX512DQ";
		case X86_64Feature::Avx512Cd:
			return "AVX512CD";
		case X86_64Feature::Avx512Bw:
			return "AVX512BW";
		case X86_64Feature::Avx512Vl:
			return "AVX512VL";
		case X86_64Feature::Avx512Ifma:
			return "AVX512_IFMA";
		case X86_64Feature::Avx512Vbmi:
			return "AVX512_VBMI";
		case X86_64Feature::Avx512Vbmi2:
			return "AVX512_VBMI2";
		case X86_64Feature::Avx512Vnni:
			return "AVX512_VNNI";
		case X86_64Feature::Avx512Bitalg:
			return "AVX512_BITALG";
		case X86_64Feature::Avx512Vpopcntdq:
			return "AVX512_VPOPCNTDQ";
		case X86_64Feature::Avx512Vp2intersect:
			return "AVX512_VP2INTERSECT";
		case X86_64Feature::Avx512Fp16:
			return "AVX512_FP16";
		case X86_64Feature::Avx512Bf16:
			return "AVX512_BF16";
		case X86_64Feature::Avx10V1:
			return "AVX10.1";
		case X86_64Feature::Avx10V2:
			return "AVX10.2";
		case X86_64Feature::ApxF:
			return "APX_F";
		case X86_64Feature::Xsave:
			return "XSAVE";
		case X86_64Feature::Osxsave:
			return "OSXSAVE";
		case X86_64Feature::Xsaveopt:
			return "XSAVEOPT";
		case X86_64Feature::Xsavec:
			return "XSAVEC";
		case X86_64Feature::Pku:
			return "PKU";
		case X86_64Feature::Rdtscp:
			return "RDTSCP";
		case X86_64Feature::InvariantTsc:
			return "Invariant TSC";
		case X86_64Feature::HypervisorPresent:
			return "Hypervisor present";
		case X86_64Feature::Hybrid:
			return "Hybrid";
		case X86_64Feature::Count:
			break;
	}

	TRIVIAL_UNREACHABLE();
}
#endif // TRIVIAL_ENABLE_LOGGING

namespace detail {

[[nodiscard]] consteval FeatureSet<X86_64Feature> targetFeatures() noexcept {
	constexpr std::array kVersion2{
	    X86_64Feature::Sse3,
	    X86_64Feature::Ssse3,
	    X86_64Feature::Sse41,
	    X86_64Feature::Sse42,
	    X86_64Feature::Popcnt,
	    X86_64Feature::Cmpxchg16b,
	    X86_64Feature::LahfSahf,
	};
	constexpr std::array kVersion3{
	    X86_64Feature::Avx,
	    X86_64Feature::Avx2,
	    X86_64Feature::Bmi1,
	    X86_64Feature::Bmi2,
	    X86_64Feature::Fma,
	    X86_64Feature::F16c,
	    X86_64Feature::Lzcnt,
	    X86_64Feature::Movbe,
	    X86_64Feature::Xsave,
	    X86_64Feature::Osxsave,
	};
	FeatureSet<X86_64Feature> features;

	for (const X86_64Feature kFeature : kVersion2) {
		features.insert(kFeature);
	}

	if constexpr (TRIVIAL_CPU_X86_64_VERSION >= 3) {
		for (const X86_64Feature kFeature : kVersion3) {
			features.insert(kFeature);
		}
	}

	return features;
}

[[nodiscard]] consteval FeatureSet<X86_64Feature> compilerTargetFeatures() noexcept {
	FeatureSet<X86_64Feature> features;
#ifdef __GCC_HAVE_SYNC_COMPARE_AND_SWAP_16
	features.insert(X86_64Feature::Cmpxchg16b);
#endif
#ifdef __CMPCCXADD__
	features.insert(X86_64Feature::Cmpccxadd);
#endif
#ifdef __RAOINT__
	features.insert(X86_64Feature::RaoInt);
#endif
#ifdef __RTM__
	features.insert(X86_64Feature::Rtm);
#endif
#ifdef __MOVDIRI__
	features.insert(X86_64Feature::Movdiri);
#endif
#ifdef __MOVDIR64B__
	features.insert(X86_64Feature::Movdir64b);
#endif
#ifdef __SERIALIZE__
	features.insert(X86_64Feature::Serialize);
#endif
#ifdef __WAITPKG__
	features.insert(X86_64Feature::Waitpkg);
#endif
#ifdef __MWAITX__
	features.insert(X86_64Feature::Monitorx);
#endif
#ifdef __CLFLUSHOPT__
	features.insert(X86_64Feature::Clflushopt);
#endif
#ifdef __CLWB__
	features.insert(X86_64Feature::Clwb);
#endif
#ifdef __CLDEMOTE__
	features.insert(X86_64Feature::Cldemote);
#endif
#ifdef __CLZERO__
	features.insert(X86_64Feature::Clzero);
#endif
#ifdef __PRFCHW__
	features.insert(X86_64Feature::Prefetchw);
#endif
#ifdef __PREFETCHI__
	features.insert(X86_64Feature::Prefetchi);
#endif
#ifdef __MOVRS__
	features.insert(X86_64Feature::Movrs);
#endif
#ifdef __PCLMUL__
	features.insert(X86_64Feature::Pclmulqdq);
#endif
#ifdef __AES__
	features.insert(X86_64Feature::Aesni);
#endif
#ifdef __SHA__
	features.insert(X86_64Feature::Sha);
#endif
#ifdef __SHA512__
	features.insert(X86_64Feature::Sha512);
#endif
#ifdef __SM3__
	features.insert(X86_64Feature::Sm3);
#endif
#ifdef __SM4__
	features.insert(X86_64Feature::Sm4);
#endif
#ifdef __GFNI__
	features.insert(X86_64Feature::Gfni);
#endif
#ifdef __VAES__
	features.insert(X86_64Feature::Vaes);
#endif
#ifdef __VPCLMULQDQ__
	features.insert(X86_64Feature::Vpclmulqdq);
#endif
#ifdef __RDRND__
	features.insert(X86_64Feature::Rdrand);
#endif
#ifdef __RDSEED__
	features.insert(X86_64Feature::Rdseed);
#endif
#ifdef __POPCNT__
	features.insert(X86_64Feature::Popcnt);
#endif
#ifdef __LZCNT__
	features.insert(X86_64Feature::Lzcnt);
#endif
#ifdef __BMI__
	features.insert(X86_64Feature::Bmi1);
#endif
#ifdef __BMI2__
	features.insert(X86_64Feature::Bmi2);
#endif
#ifdef __TBM__
	features.insert(X86_64Feature::Tbm);
#endif
#ifdef __ADX__
	features.insert(X86_64Feature::Adx);
#endif
#ifdef __MOVBE__
	features.insert(X86_64Feature::Movbe);
#endif
#ifdef __LAHF_SAHF__
	features.insert(X86_64Feature::LahfSahf);
#endif
#ifdef __RDPID__
	features.insert(X86_64Feature::Rdpid);
#endif
#ifdef __SSE3__
	features.insert(X86_64Feature::Sse3);
#endif
#ifdef __SSSE3__
	features.insert(X86_64Feature::Ssse3);
#endif
#ifdef __SSE4_1__
	features.insert(X86_64Feature::Sse41);
#endif
#ifdef __SSE4_2__
	features.insert(X86_64Feature::Sse42);
#endif
#ifdef __SSE4A__
	features.insert(X86_64Feature::Sse4a);
#endif
#ifdef __AVX__
	features.insert(X86_64Feature::Avx);
#endif
#ifdef __AVX2__
	features.insert(X86_64Feature::Avx2);
#endif
#ifdef __FMA__
	features.insert(X86_64Feature::Fma);
#endif
#ifdef __F16C__
	features.insert(X86_64Feature::F16c);
#endif
#ifdef __AVXVNNI__
	features.insert(X86_64Feature::AvxVnni);
#endif
#ifdef __AVXVNNIINT8__
	features.insert(X86_64Feature::AvxVnniInt8);
#endif
#ifdef __AVXVNNIINT16__
	features.insert(X86_64Feature::AvxVnniInt16);
#endif
#ifdef __AVXIFMA__
	features.insert(X86_64Feature::AvxIfma);
#endif
#ifdef __AVXNECONVERT__
	features.insert(X86_64Feature::AvxNeConvert);
#endif
#ifdef __XOP__
	features.insert(X86_64Feature::Xop);
#endif
#ifdef __FMA4__
	features.insert(X86_64Feature::Fma4);
#endif
#ifdef __AVX512F__
	features.insert(X86_64Feature::Avx512F);
#endif
#ifdef __AVX512DQ__
	features.insert(X86_64Feature::Avx512Dq);
#endif
#ifdef __AVX512CD__
	features.insert(X86_64Feature::Avx512Cd);
#endif
#ifdef __AVX512BW__
	features.insert(X86_64Feature::Avx512Bw);
#endif
#ifdef __AVX512VL__
	features.insert(X86_64Feature::Avx512Vl);
#endif
#ifdef __AVX512IFMA__
	features.insert(X86_64Feature::Avx512Ifma);
#endif
#ifdef __AVX512VBMI__
	features.insert(X86_64Feature::Avx512Vbmi);
#endif
#ifdef __AVX512VBMI2__
	features.insert(X86_64Feature::Avx512Vbmi2);
#endif
#ifdef __AVX512VNNI__
	features.insert(X86_64Feature::Avx512Vnni);
#endif
#ifdef __AVX512BITALG__
	features.insert(X86_64Feature::Avx512Bitalg);
#endif
#ifdef __AVX512VPOPCNTDQ__
	features.insert(X86_64Feature::Avx512Vpopcntdq);
#endif
#ifdef __AVX512VP2INTERSECT__
	features.insert(X86_64Feature::Avx512Vp2intersect);
#endif
#ifdef __AVX512FP16__
	features.insert(X86_64Feature::Avx512Fp16);
#endif
#ifdef __AVX512BF16__
	features.insert(X86_64Feature::Avx512Bf16);
#endif
#ifdef __AVX10_1__
	features.insert(X86_64Feature::Avx10V1);
#endif
#ifdef __AVX10_2__
	features.insert(X86_64Feature::Avx10V2);
#endif
#ifdef __APX_F__
	features.insert(X86_64Feature::ApxF);
#endif
#ifdef __XSAVE__
	features.insert(X86_64Feature::Xsave);
#endif
#ifdef __XSAVEOPT__
	features.insert(X86_64Feature::Xsaveopt);
#endif
#ifdef __XSAVEC__
	features.insert(X86_64Feature::Xsavec);
#endif
#ifdef __PKU__
	features.insert(X86_64Feature::Pku);
#endif
	return features;
}

static_assert(
    targetFeatures().containsAll(compilerTargetFeatures()),
    "The compiler targets CPU features beyond the build's CPU version, which the floor check would not cover");

} // namespace detail

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr bool isTargetFeature(X86_64Feature feature) noexcept {
	return detail::targetFeatures().contains(feature);
}

} // namespace trivial::cpu

#endif // TRIVIAL_ARCH_X86_64

#endif // TRIVIAL_CORE_CPU_CPU_FEATURES_X86_64_H

#ifndef TRIVIAL_SRC_CORE_CPU_CPU_PROBE_ARM64_H
#define TRIVIAL_SRC_CORE_CPU_CPU_PROBE_ARM64_H

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>

#include <trivial/core/assert.h>
#include <trivial/core/cpu/cpu.h>
#include <trivial/core/cpu/cpu_feature_set.h>
#include <trivial/core/cpu/cpu_features_arm64.h>

#include "core/cpu/cpu_probe.h"

#define TRIVIAL_CPU_ARM64_IMPLEMENTER_ARM std::uint8_t{0x41}
#define TRIVIAL_CPU_ARM64_IMPLEMENTER_BROADCOM std::uint8_t{0x42}
#define TRIVIAL_CPU_ARM64_IMPLEMENTER_CAVIUM std::uint8_t{0x43}
#define TRIVIAL_CPU_ARM64_IMPLEMENTER_FUJITSU std::uint8_t{0x46}
#define TRIVIAL_CPU_ARM64_IMPLEMENTER_HISILICON std::uint8_t{0x48}
#define TRIVIAL_CPU_ARM64_IMPLEMENTER_NVIDIA std::uint8_t{0x4E}
#define TRIVIAL_CPU_ARM64_IMPLEMENTER_APM std::uint8_t{0x50}
#define TRIVIAL_CPU_ARM64_IMPLEMENTER_QUALCOMM std::uint8_t{0x51}
#define TRIVIAL_CPU_ARM64_IMPLEMENTER_SAMSUNG std::uint8_t{0x53}
#define TRIVIAL_CPU_ARM64_IMPLEMENTER_MARVELL std::uint8_t{0x56}
#define TRIVIAL_CPU_ARM64_IMPLEMENTER_APPLE std::uint8_t{0x61}
#define TRIVIAL_CPU_ARM64_IMPLEMENTER_ARM_CHINA std::uint8_t{0x63}
#define TRIVIAL_CPU_ARM64_IMPLEMENTER_MICROSOFT std::uint8_t{0x6D}
#define TRIVIAL_CPU_ARM64_IMPLEMENTER_PHYTIUM std::uint8_t{0x70}
#define TRIVIAL_CPU_ARM64_IMPLEMENTER_AMPERE std::uint8_t{0xC0}

#define TRIVIAL_CPU_ARM64_MIDR_REVISION_LOW_BIT 0U
#define TRIVIAL_CPU_ARM64_MIDR_REVISION_HIGH_BIT 3U
#define TRIVIAL_CPU_ARM64_MIDR_PART_LOW_BIT 4U
#define TRIVIAL_CPU_ARM64_MIDR_PART_HIGH_BIT 15U
#define TRIVIAL_CPU_ARM64_MIDR_VARIANT_LOW_BIT 20U
#define TRIVIAL_CPU_ARM64_MIDR_VARIANT_HIGH_BIT 23U
#define TRIVIAL_CPU_ARM64_MIDR_IMPLEMENTER_LOW_BIT 24U
#define TRIVIAL_CPU_ARM64_MIDR_IMPLEMENTER_HIGH_BIT 31U

namespace trivial::cpu {

[[nodiscard]] constexpr FeatureSet<Arm64Feature> featurePrerequisites(Arm64Feature feature) noexcept {
	FeatureSet<Arm64Feature> prerequisites;

	switch (feature) {
		case Arm64Feature::Rpres:
			prerequisites.insert(Arm64Feature::Afp);
			return prerequisites;

		case Arm64Feature::Ebf16:
			prerequisites.insert(Arm64Feature::Bf16);
			return prerequisites;

		case Arm64Feature::Fp8:
		case Arm64Feature::Fp8Fma:
		case Arm64Feature::Fp8Dot4:
		case Arm64Feature::Fp8Dot2:
		case Arm64Feature::F8f16mm:
		case Arm64Feature::F8f32mm:
			prerequisites.insert(Arm64Feature::Fpmr);
			return prerequisites;

		case Arm64Feature::Sve2:
		case Arm64Feature::Sve2p1:
		case Arm64Feature::Sve2p2:
		case Arm64Feature::Sve2p3:
		case Arm64Feature::SveF32mm:
		case Arm64Feature::SveF64mm:
		case Arm64Feature::SveF16f32mm:
		case Arm64Feature::SveB16b16:
		case Arm64Feature::SveB16mm:
		case Arm64Feature::SveBfscale:
		case Arm64Feature::SveAes:
		case Arm64Feature::SvePmull128:
		case Arm64Feature::SveAes2:
		case Arm64Feature::SveBitperm:
		case Arm64Feature::SveSha3:
		case Arm64Feature::SveSm4:
		case Arm64Feature::SvePerm256:
		case Arm64Feature::SveLut6:
			prerequisites.insert(Arm64Feature::Sve);
			return prerequisites;

		case Arm64Feature::SveI8mm:
			prerequisites.insert(Arm64Feature::I8mm);
			prerequisites.insert(Arm64Feature::Sve);
			return prerequisites;

		case Arm64Feature::SveBf16:
			prerequisites.insert(Arm64Feature::Bf16);
			prerequisites.insert(Arm64Feature::Sve);
			return prerequisites;

		case Arm64Feature::SveEbf16:
			prerequisites.insert(Arm64Feature::Bf16);
			prerequisites.insert(Arm64Feature::Sve);
			prerequisites.insert(Arm64Feature::SveBf16);
			return prerequisites;

		case Arm64Feature::Sme2:
		case Arm64Feature::Sme2p1:
		case Arm64Feature::Sme2p2:
		case Arm64Feature::Sme2p3:
		case Arm64Feature::SmeF64F64:
		case Arm64Feature::SmeI16I64:
		case Arm64Feature::SmeF16F16:
		case Arm64Feature::SmeB16B16:
		case Arm64Feature::SmeLut6:
		case Arm64Feature::SmeTmop:
		case Arm64Feature::SmeMop4:
		case Arm64Feature::SmeFa64:
		case Arm64Feature::SsveAes:
		case Arm64Feature::SsveBitperm:
		case Arm64Feature::SsveFexpa:
			prerequisites.insert(Arm64Feature::Sme);
			return prerequisites;

		case Arm64Feature::SmeF8F16:
		case Arm64Feature::SmeF8F32:
		case Arm64Feature::SsveFp8Fma:
		case Arm64Feature::SsveFp8Dot4:
		case Arm64Feature::SsveFp8Dot2:
		case Arm64Feature::SsveF8f16mm:
		case Arm64Feature::SsveF8f32mm:
			prerequisites.insert(Arm64Feature::Fpmr);
			prerequisites.insert(Arm64Feature::Sme);
			return prerequisites;

		case Arm64Feature::SmeLutv2:
			prerequisites.insert(Arm64Feature::Sme);
			prerequisites.insert(Arm64Feature::Sme2);
			return prerequisites;

		case Arm64Feature::Mte2:
			prerequisites.insert(Arm64Feature::Mte);
			return prerequisites;

		case Arm64Feature::Mte3:
		case Arm64Feature::MteAsync:
		case Arm64Feature::MteStoreOnly:
		case Arm64Feature::MteFar:
			prerequisites.insert(Arm64Feature::Mte);
			prerequisites.insert(Arm64Feature::Mte2);
			return prerequisites;

		case Arm64Feature::Lse:
		case Arm64Feature::Lse2:
		case Arm64Feature::Lse128:
		case Arm64Feature::Lsfe:
		case Arm64Feature::Lrcpc:
		case Arm64Feature::Lrcpc2:
		case Arm64Feature::Lrcpc3:
		case Arm64Feature::Mops:
		case Arm64Feature::Rprfm:
		case Arm64Feature::Wfxt:
		case Arm64Feature::Crc32:
		case Arm64Feature::Aes:
		case Arm64Feature::Pmull:
		case Arm64Feature::Sha1:
		case Arm64Feature::Sha256:
		case Arm64Feature::Sha3:
		case Arm64Feature::Sha512:
		case Arm64Feature::Sm3:
		case Arm64Feature::Sm4:
		case Arm64Feature::Rng:
		case Arm64Feature::Cssc:
		case Arm64Feature::FlagM:
		case Arm64Feature::FlagM2:
		case Arm64Feature::Jscvt:
		case Arm64Feature::Hbc:
		case Arm64Feature::Cmpbr:
		case Arm64Feature::Frintts:
		case Arm64Feature::Fprcvt:
		case Arm64Feature::Afp:
		case Arm64Feature::FpSyncExceptions:
		case Arm64Feature::Rdm:
		case Arm64Feature::DotProd:
		case Arm64Feature::Fp16:
		case Arm64Feature::Fhm:
		case Arm64Feature::F16f32dot:
		case Arm64Feature::F16f32mm:
		case Arm64Feature::F16mm:
		case Arm64Feature::Fcma:
		case Arm64Feature::I8mm:
		case Arm64Feature::Bf16:
		case Arm64Feature::Lut:
		case Arm64Feature::Faminmax:
		case Arm64Feature::Fpmr:
		case Arm64Feature::Sve:
		case Arm64Feature::Sme:
		case Arm64Feature::Mte:
		case Arm64Feature::Poe:
		case Arm64Feature::Sb:
		case Arm64Feature::Ssbs:
		case Arm64Feature::Dit:
		case Arm64Feature::Bti:
		case Arm64Feature::PAuth:
		case Arm64Feature::Gcs:
		case Arm64Feature::Dpb:
		case Arm64Feature::Dpb2:
		case Arm64Feature::Ecv:
			return prerequisites;
		case Arm64Feature::Count:
			break;
	}

	TRIVIAL_UNREACHABLE();
}

static_assert(
    [] {
	    for (std::size_t i = 0; i < static_cast<std::size_t>(Arm64Feature::Count); ++i) {
		    const FeatureSet<Arm64Feature> kPrerequisites = featurePrerequisites(static_cast<Arm64Feature>(i));

		    for (std::size_t j = 0; j < static_cast<std::size_t>(Arm64Feature::Count); ++j) {
			    const Arm64Feature kOther = static_cast<Arm64Feature>(j);

			    if (kPrerequisites.contains(kOther) && !kPrerequisites.containsAll(featurePrerequisites(kOther))) {
				    return false;
			    }
		    }
	    }

	    return true;
    }(),
    "Every feature should list the prerequisites of its prerequisites too");

[[nodiscard]] constexpr Arm64MainId decodeMainId(std::uint64_t mainIdRegister) noexcept {
	const std::uint32_t kValue = static_cast<std::uint32_t>(mainIdRegister);

	return Arm64MainId{
	    .part = static_cast<std::uint16_t>(
	        extractBits(kValue, TRIVIAL_CPU_ARM64_MIDR_PART_HIGH_BIT, TRIVIAL_CPU_ARM64_MIDR_PART_LOW_BIT)),
	    .implementer = static_cast<std::uint8_t>(extractBits(kValue,
		                                                     TRIVIAL_CPU_ARM64_MIDR_IMPLEMENTER_HIGH_BIT,
		                                                     TRIVIAL_CPU_ARM64_MIDR_IMPLEMENTER_LOW_BIT)),
	    .variant = static_cast<std::uint8_t>(
	        extractBits(kValue, TRIVIAL_CPU_ARM64_MIDR_VARIANT_HIGH_BIT, TRIVIAL_CPU_ARM64_MIDR_VARIANT_LOW_BIT)),
	    .revision = static_cast<std::uint8_t>(
	        extractBits(kValue, TRIVIAL_CPU_ARM64_MIDR_REVISION_HIGH_BIT, TRIVIAL_CPU_ARM64_MIDR_REVISION_LOW_BIT)),
	};
}

static_assert(
    [] {
	    const Arm64MainId kCortexA76 = decodeMainId(0x414FD0B1);
	    const std::uint8_t kImplementer = 0x41;
	    const std::uint8_t kVariant = 0x4;
	    const std::uint16_t kPart = 0xD0B;
	    const std::uint8_t kRevision = 0x1;

	    return kCortexA76.implementer == kImplementer && kCortexA76.variant == kVariant && kCortexA76.part == kPart
	           && kCortexA76.revision == kRevision;
    }(),
    "decodeMainId should read every MIDR field (Cortex-A76 r4p1)");

[[nodiscard]] constexpr const char* implementerName(std::uint8_t implementer) noexcept {
	switch (implementer) {
		case TRIVIAL_CPU_ARM64_IMPLEMENTER_ARM:
			return "ARM";
		case TRIVIAL_CPU_ARM64_IMPLEMENTER_BROADCOM:
			return "Broadcom";
		case TRIVIAL_CPU_ARM64_IMPLEMENTER_CAVIUM:
			return "Cavium";
		case TRIVIAL_CPU_ARM64_IMPLEMENTER_FUJITSU:
			return "Fujitsu";
		case TRIVIAL_CPU_ARM64_IMPLEMENTER_HISILICON:
			return "HiSilicon";
		case TRIVIAL_CPU_ARM64_IMPLEMENTER_NVIDIA:
			return "NVIDIA";
		case TRIVIAL_CPU_ARM64_IMPLEMENTER_APM:
			return "APM";
		case TRIVIAL_CPU_ARM64_IMPLEMENTER_QUALCOMM:
			return "Qualcomm";
		case TRIVIAL_CPU_ARM64_IMPLEMENTER_SAMSUNG:
			return "Samsung";
		case TRIVIAL_CPU_ARM64_IMPLEMENTER_MARVELL:
			return "Marvell";
		case TRIVIAL_CPU_ARM64_IMPLEMENTER_APPLE:
			return "Apple";
		case TRIVIAL_CPU_ARM64_IMPLEMENTER_ARM_CHINA:
			return "Arm China";
		case TRIVIAL_CPU_ARM64_IMPLEMENTER_MICROSOFT:
			return "Microsoft";
		case TRIVIAL_CPU_ARM64_IMPLEMENTER_PHYTIUM:
			return "Phytium";
		case TRIVIAL_CPU_ARM64_IMPLEMENTER_AMPERE:
			return "Ampere";
		default:
			return "";
	}
}

static_assert(
    [] {
	    for (std::size_t i = 0; i <= std::numeric_limits<std::uint8_t>::max(); ++i) {
		    if (std::string_view{implementerName(static_cast<std::uint8_t>(i))}.size() >= sizeof(VendorString)) {
			    return false;
		    }
	    }

	    return true;
    }(),
    "Every implementer name should fit the vendor string with its terminator");

} // namespace trivial::cpu

#undef TRIVIAL_CPU_ARM64_MIDR_REVISION_LOW_BIT

#undef TRIVIAL_CPU_ARM64_IMPLEMENTER_ARM
#undef TRIVIAL_CPU_ARM64_IMPLEMENTER_BROADCOM
#undef TRIVIAL_CPU_ARM64_IMPLEMENTER_CAVIUM
#undef TRIVIAL_CPU_ARM64_IMPLEMENTER_FUJITSU
#undef TRIVIAL_CPU_ARM64_IMPLEMENTER_HISILICON
#undef TRIVIAL_CPU_ARM64_IMPLEMENTER_NVIDIA
#undef TRIVIAL_CPU_ARM64_IMPLEMENTER_APM
#undef TRIVIAL_CPU_ARM64_IMPLEMENTER_QUALCOMM
#undef TRIVIAL_CPU_ARM64_IMPLEMENTER_SAMSUNG
#undef TRIVIAL_CPU_ARM64_IMPLEMENTER_MARVELL
#undef TRIVIAL_CPU_ARM64_IMPLEMENTER_APPLE
#undef TRIVIAL_CPU_ARM64_IMPLEMENTER_ARM_CHINA
#undef TRIVIAL_CPU_ARM64_IMPLEMENTER_MICROSOFT
#undef TRIVIAL_CPU_ARM64_IMPLEMENTER_PHYTIUM
#undef TRIVIAL_CPU_ARM64_IMPLEMENTER_AMPERE
#undef TRIVIAL_CPU_ARM64_MIDR_REVISION_HIGH_BIT
#undef TRIVIAL_CPU_ARM64_MIDR_PART_LOW_BIT
#undef TRIVIAL_CPU_ARM64_MIDR_PART_HIGH_BIT
#undef TRIVIAL_CPU_ARM64_MIDR_VARIANT_LOW_BIT
#undef TRIVIAL_CPU_ARM64_MIDR_VARIANT_HIGH_BIT
#undef TRIVIAL_CPU_ARM64_MIDR_IMPLEMENTER_LOW_BIT
#undef TRIVIAL_CPU_ARM64_MIDR_IMPLEMENTER_HIGH_BIT

#endif // TRIVIAL_SRC_CORE_CPU_CPU_PROBE_ARM64_H

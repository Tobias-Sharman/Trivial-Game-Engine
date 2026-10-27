#include <trivial/core/cpu/cpu.h>

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include <trivial/core/cpu/cpu_config.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX

#include <windows.h> // IWYU pragma: keep

#define TRIVIAL_CPU_WINDOWS_CPU_RATE_SCALE std::uint64_t{10'000}

namespace trivial::cpu {

[[nodiscard]] std::uint32_t availableLogicalProcessorCount() noexcept {
	DWORD_PTR processMask = 0;
	DWORD_PTR systemMask = 0;

	if (GetProcessAffinityMask(GetCurrentProcess(), &processMask, &systemMask) != 0 && processMask != 0) {
		if (GetActiveProcessorGroupCount() == 1) {
			return static_cast<std::uint32_t>(std::popcount(processMask));
		}

		USHORT processGroup = 0;
		USHORT processGroupCount = 1;

		if (processMask != systemMask
		    && GetProcessGroupAffinity(GetCurrentProcess(), &processGroupCount, &processGroup) != 0
		    && processGroupCount == 1) {
			return static_cast<std::uint32_t>(std::popcount(processMask));
		}
	}

	std::vector<GROUP_AFFINITY> jobGroups(GetActiveProcessorGroupCount());
	DWORD jobGroupsSize = 0;

	if (!jobGroups.empty()
	    && QueryInformationJobObject(nullptr,
	                                 JobObjectGroupInformationEx,
	                                 jobGroups.data(),
	                                 static_cast<DWORD>(jobGroups.size() * sizeof(GROUP_AFFINITY)),
	                                 &jobGroupsSize)
	           != 0) {
		const std::size_t kJobGroupCount = std::min(jobGroupsSize / sizeof(GROUP_AFFINITY), jobGroups.size());
		std::uint32_t count = 0;

		for (const GROUP_AFFINITY& kGroup : std::span{jobGroups}.first(kJobGroupCount)) {
			count += static_cast<std::uint32_t>(std::popcount(kGroup.Mask));
		}

		if (count != 0) {
			return count;
		}
	}

	return std::max(static_cast<std::uint32_t>(GetActiveProcessorCount(ALL_PROCESSOR_GROUPS)), std::uint32_t{1});
}

[[nodiscard]] std::uint32_t quotaLogicalProcessorCount() noexcept {
	const std::uint32_t kAvailable = availableLogicalProcessorCount();
	JOBOBJECT_CPU_RATE_CONTROL_INFORMATION rateControl{};

	if (QueryInformationJobObject(nullptr,
	                              JobObjectCpuRateControlInformation,
	                              &rateControl,
	                              sizeof(rateControl),
	                              nullptr)
	    == 0) {
		return kAvailable;
	}

	const DWORD kFlags = rateControl.ControlFlags;
	std::uint64_t rate = 0;

	if ((kFlags & JOB_OBJECT_CPU_RATE_CONTROL_ENABLE) != 0) {
		if ((kFlags & JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP) != 0) {
			rate = rateControl.CpuRate; // NOLINT(cppcoreguidelines-pro-type-union-access)
		} else if ((kFlags & JOB_OBJECT_CPU_RATE_CONTROL_MIN_MAX_RATE) != 0) {
			rate = rateControl.MaxRate; // NOLINT(cppcoreguidelines-pro-type-union-access)
		}
	}

	const std::uint64_t kTotal = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);

	if (rate == 0 || kTotal == 0) {
		return kAvailable;
	}

	const std::uint64_t kRounding = TRIVIAL_CPU_ROUND_QUOTA_UP ? TRIVIAL_CPU_WINDOWS_CPU_RATE_SCALE - 1 : 0;
	const std::uint64_t kQuota = ((rate * kTotal) + kRounding) / TRIVIAL_CPU_WINDOWS_CPU_RATE_SCALE;

	return static_cast<std::uint32_t>(std::clamp(kQuota, std::uint64_t{1}, std::uint64_t{kAvailable}));
}

} // namespace trivial::cpu

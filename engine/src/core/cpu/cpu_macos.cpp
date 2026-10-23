#include <trivial/core/cpu/cpu.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <sys/sysctl.h>

namespace trivial::cpu {

[[nodiscard]] std::uint32_t availableLogicalProcessorCount() noexcept {
	std::uint64_t activeCount = 0;
	std::size_t size = sizeof(activeCount);
	(void)sysctlbyname("hw.activecpu", &activeCount, &size, nullptr, 0);
	return static_cast<std::uint32_t>(std::max(activeCount, std::uint64_t{1}));
}

[[nodiscard]] std::uint32_t quotaLogicalProcessorCount() noexcept {
	return availableLogicalProcessorCount();
}

} // namespace trivial::cpu

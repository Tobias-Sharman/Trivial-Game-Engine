#include <trivial/core/cpu/cpu.h>

#include <algorithm>
#include <bit>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <sched.h>
#include <unistd.h>
#include <vector>

#include <trivial/core/log.h>

#define TRIVIAL_CPU_LINUX_AFFINITY_INITIAL_WORDS std::size_t{16}
#define TRIVIAL_CPU_LINUX_AFFINITY_MAX_WORDS (std::size_t{1} << 16U)

namespace trivial::cpu {

[[nodiscard]] std::uint32_t availableLogicalProcessorCount() noexcept {
	TRIVIAL_LOG_INFO_PREFIX("Cpu",
	                        "the available count reads the calling thread's affinity on Linux, so it needs replacing "
	                        "with a start-up read before threads are pinned");

	std::vector<std::uint64_t> words(TRIVIAL_CPU_LINUX_AFFINITY_INITIAL_WORDS);

	while (words.size() <= TRIVIAL_CPU_LINUX_AFFINITY_MAX_WORDS) {
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		if (sched_getaffinity(0, words.size() * sizeof(std::uint64_t), reinterpret_cast<cpu_set_t*>(words.data()))
		    == 0) {
			std::uint32_t count = 0;

			for (const std::uint64_t kWord : words) {
				count += static_cast<std::uint32_t>(std::popcount(kWord));
			}

			return std::max(count, std::uint32_t{1});
		}

		if (errno != EINVAL) {
			break;
		}

		words.resize(words.size() * 2);
	}

	return static_cast<std::uint32_t>(std::max(sysconf(_SC_NPROCESSORS_ONLN), 1L));
}

[[nodiscard]] std::uint32_t quotaLogicalProcessorCount() noexcept {
	TRIVIAL_LOG_INFO_PREFIX("Cpu", "cgroup CPU limits are not read on Linux yet, so the quota is the available count");

	return availableLogicalProcessorCount();
}

} // namespace trivial::cpu

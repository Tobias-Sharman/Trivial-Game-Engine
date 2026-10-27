#include "core/cpu/cpu_probe.h"

#include <trivial/core/cpu/cpu.h>
#include <trivial/core/log.h>

namespace trivial::cpu {

[[nodiscard]] RawTopology probeRawTopology() noexcept {
	TRIVIAL_LOG_INFO_PREFIX("Cpu", "the topology is not probed on Linux yet, so it is unknown");

	return RawTopology{
	    .logicalProcessors = nullptr,
	    .caches = nullptr,
	    .logicalProcessorCount = 0,
	    .cacheCount = 0,
	    .cacheLineSize = 0,
	};
}

void probeRunningTranslated() noexcept {
	TRIVIAL_LOG_INFO_PREFIX("Cpu", "running translated is not detected on Linux yet, so it is reported as false");
	detail::g_detectedCapabilities.runningTranslated = false;
}

} // namespace trivial::cpu

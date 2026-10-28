#include <gtest/gtest.h>

#include <trivial/core/cpu/cpu.h>
#include <trivial/core/log.h>
#include <trivial/core/platform.h>

#include "core/cpu/cpu_capabilities.h"
#include "core/memory/memory_capabilities.h"
#include "core/time/time_capabilities.h"

int main(int argc, char** argv) {
	::testing::InitGoogleTest(&argc, argv);

	trivial::memory::initCapabilities();
	trivial::cpu::initCapabilities();
	trivial::time::initCapabilities();

	TRIVIAL_LOG_STARTUP_SYSTEM_INFO();

	const int kResult = RUN_ALL_TESTS();

#if TRIVIAL_PLATFORM_HAS_CPU_AFFINITY
	delete[] trivial::cpu::detail::g_detectedCapabilities.topology.logicalProcessors;
	delete[] trivial::cpu::detail::g_detectedCapabilities.topology.caches;
#else
	delete[] trivial::cpu::detail::g_detectedCapabilities.topology.efficiencyClasses;
#endif // TRIVIAL_PLATFORM_HAS_CPU_AFFINITY
	trivial::cpu::detail::g_detectedCapabilities.topology = {};

	return kResult;
}

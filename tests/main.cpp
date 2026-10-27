#include <gtest/gtest.h>

#include <trivial/core/log.h>

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

	trivial::cpu::freeCapabilities();

	return kResult;
}

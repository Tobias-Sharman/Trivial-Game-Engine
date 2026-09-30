#include <gtest/gtest.h>

#include "core/memory/memory_capabilities.h"

int main(int argc, char** argv) {
	::testing::InitGoogleTest(&argc, argv);

	trivial::memory::initCapabilities();

	return RUN_ALL_TESTS();
}

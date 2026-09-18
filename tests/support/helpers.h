#ifndef TESTS_SUPPORT_HELPERS_H
#define TESTS_SUPPORT_HELPERS_H

#include <cstddef>
#include <cstdlib>

#include <gtest/gtest.h>

#include <trivial/core/log.h>
#include <trivial/core/thread/thread.h>

#include "core/heap_array.h"
#include "core/sync/parking_lot.h"

#if TRIVIAL_PLATFORM_POSIX
#include <trivial/core/thread/thread_stack_allocator.h>
#endif // TRIVIAL_PLATFORM_POSIX

namespace trivial::tests {

#if TRIVIAL_PLATFORM_POSIX
inline void attachStackAllocator(trivial::thread::ThreadConfig& config) {
	static trivial::thread::ThreadStackAllocator s_allocator;
	config.stackAllocator = &s_allocator;
}
#endif // TRIVIAL_PLATFORM_POSIX

// Tests creating threads or a parking lot should be ran isolated to not mess
// with the global counter on thread index and introduce a subtle error
inline void requireIsolatedProcess() noexcept {
	if (::testing::UnitTest::GetInstance()->test_to_run_count() != 1) {
		TRIVIAL_LOG_FATAL_PREFIX("Tests",
		                         "tests that create threads must run one per process - run via scripts/build.py "
		                         "--test (and add this test to its ISOLATED_TESTS list if it's not there yet), not "
		                         "the raw binary with more than one test selected");
		std::abort();
	}
}

class ScopedParkingLot {
public:
	explicit ScopedParkingLot(std::size_t threadCount) noexcept
	    : m_parkingLot(1 + threadCount) {
		requireIsolatedProcess();

		trivial::sync::setActiveParkingLot(&m_parkingLot);
	}

	~ScopedParkingLot() noexcept { trivial::sync::setActiveParkingLot(nullptr); }

	ScopedParkingLot(const ScopedParkingLot&) = delete;
	ScopedParkingLot& operator=(const ScopedParkingLot&) = delete;

	ScopedParkingLot(ScopedParkingLot&&) = delete;
	ScopedParkingLot& operator=(ScopedParkingLot&&) = delete;

private:
	trivial::sync::ParkingLot m_parkingLot;
};

inline void runOnAllThreads(std::size_t threadCount, trivial::thread::ThreadStartRoutine routine, void* arg) {
	ScopedParkingLot parkingLotScope(threadCount);

	trivial::core::HeapArray<trivial::thread::Thread> threads(threadCount);

	for (std::size_t i = 0; i < threadCount; ++i) {
		trivial::thread::ThreadConfig config;
#if TRIVIAL_PLATFORM_POSIX
		attachStackAllocator(config);
#endif // TRIVIAL_PLATFORM_POSIX

		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
		trivial::thread::ThreadCreateResult result = threads[i].create(config, routine, arg);
		ASSERT_EQ(result.error, trivial::thread::ThreadCreateError::None);
	}

	for (std::size_t i = 0; i < threadCount; ++i) {
		threads[i].join(); // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
	}
}

} // namespace trivial::tests

#endif // TESTS_SUPPORT_HELPERS_H

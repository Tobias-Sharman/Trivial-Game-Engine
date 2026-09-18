#ifndef TESTS_SUPPORT_HELPERS_H
#define TESTS_SUPPORT_HELPERS_H

#include <array>
#include <cstddef>

#include <gtest/gtest.h>

#include <trivial/core/thread/thread.h>

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

template <std::size_t ThreadCount>
void runOnAllThreads(trivial::thread::ThreadStartRoutine routine, void* arg) {
	std::array<trivial::thread::Thread, ThreadCount> threads;

	for (trivial::thread::Thread& thread : threads) {
		trivial::thread::ThreadConfig config;
#if TRIVIAL_PLATFORM_POSIX
		attachStackAllocator(config);
#endif // TRIVIAL_PLATFORM_POSIX

		trivial::thread::ThreadCreateResult result = thread.create(config, routine, arg);
		ASSERT_EQ(result.error, trivial::thread::ThreadCreateError::None);
	}

	for (trivial::thread::Thread& thread : threads) {
		thread.join();
	}
}

} // namespace trivial::tests

#endif // TESTS_SUPPORT_HELPERS_H

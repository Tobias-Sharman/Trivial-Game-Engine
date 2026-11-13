#ifndef TRIVIAL_CORE_THREAD_THREAD_STACK_H
#define TRIVIAL_CORE_THREAD_THREAD_STACK_H

#include <trivial/core/platform.h>

#if TRIVIAL_PLATFORM_POSIX

#include <trivial/core/memory/byte_count.h>

namespace trivial::thread {

struct ThreadStackAllocation {
	void* stackBase = nullptr;

	memory::ByteCount stackSize;
	memory::ByteCount guardSize;
};

[[nodiscard]] bool allocateStack(memory::ByteCount stackSize,
                                 ThreadStackAllocation& outAllocation,
                                 int& outOsErrorCode) noexcept;

void releaseStack(const ThreadStackAllocation& allocation) noexcept;

} // namespace trivial::thread

#endif // TRIVIAL_PLATFORM_POSIX

#endif // TRIVIAL_CORE_THREAD_THREAD_STACK_H

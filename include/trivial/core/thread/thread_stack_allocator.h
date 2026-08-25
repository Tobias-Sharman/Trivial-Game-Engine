#ifndef TRIVIAL_CORE_THREAD_THREAD_STACK_ALLOCATOR_H
#define TRIVIAL_CORE_THREAD_THREAD_STACK_ALLOCATOR_H

#include <trivial/core/platform.h>

#if TRIVIAL_PLATFORM_POSIX

#include <cstddef>

namespace trivial::thread {

struct ThreadStackAllocation {
	void* stackBase = nullptr;
	std::size_t stackSize = 0;

	std::size_t guardSize = 0;
};

// allocate and release are not thread safe, so just keep to calling from one
// thread when spawning or joining workers
class ThreadStackAllocator {
public:
	ThreadStackAllocator() noexcept;
	~ThreadStackAllocator() noexcept;

	ThreadStackAllocator(const ThreadStackAllocator&) = delete;
	ThreadStackAllocator& operator=(const ThreadStackAllocator&) = delete;

	ThreadStackAllocator(ThreadStackAllocator&&) = delete;
	ThreadStackAllocator& operator=(ThreadStackAllocator&&) = delete;

	[[nodiscard]] bool allocate(std::size_t stackSize,
	                            ThreadStackAllocation& outAllocation,
	                            int& outOsErrorCode) noexcept;

	void release(const ThreadStackAllocation& allocation) noexcept;

	[[nodiscard]] std::size_t committedBytes() const noexcept { return m_committedBytes; }

private:
	std::size_t m_pageSize = 0;
	std::size_t m_allocationGranularity = 0;

	std::size_t m_committedBytes = 0;
};

} // namespace trivial::thread

#else
#error "ThreadStackAllocator is POSIX only"

#endif // TRIVIAL_PLATFORM_POSIX

#endif // TRIVIAL_CORE_THREAD_THREAD_STACK_ALLOCATOR_H

#include <trivial/core/thread/thread_stack_allocator.h>

#if TRIVIAL_PLATFORM_POSIX

#include <trivial/core/assert.h>

#include "core/memory/virtual_memory.h"

namespace trivial::thread {

ThreadStackAllocator::ThreadStackAllocator() noexcept {
	const memory::SystemInfo kSystemInfo = memory::probeSystemInfo();
	m_pageSize = kSystemInfo.pageSize;
	m_allocationGranularity = kSystemInfo.allocationGranularity;
}

ThreadStackAllocator::~ThreadStackAllocator() noexcept {
	TRIVIAL_ASSERT(m_committedBytes == 0);
}

[[nodiscard]] bool ThreadStackAllocator::allocate(std::size_t stackSize,
                                                  ThreadStackAllocation& outAllocation,
                                                  int& outOsErrorCode) noexcept {
	TRIVIAL_ASSERT(stackSize > 0);

	const memory::SystemInfo kSystemInfo{.pageSize = m_pageSize, .allocationGranularity = m_allocationGranularity};
	const std::size_t kPageSize = m_pageSize;
	const std::size_t kAlignment = m_allocationGranularity;

	const std::size_t kGuardBytes = kPageSize;
	const std::size_t kRequestedStackBytes = (stackSize + kPageSize - 1) & ~(kPageSize - 1);

	std::size_t totalBytes = kGuardBytes + kRequestedStackBytes;
	totalBytes = (totalBytes + kAlignment - 1) & ~(kAlignment - 1);

	const std::size_t kStackBytes = totalBytes - kGuardBytes;

	void* const kBase = memory::reserveAligned(totalBytes, kAlignment, kSystemInfo, outOsErrorCode);

	if (kBase == nullptr) {
		return false;
	}

	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	void* const kStackBase = static_cast<char*>(kBase) + kGuardBytes;

	if (!memory::commitPages(kStackBase, kStackBytes, kPageSize, outOsErrorCode)) {
		memory::releaseReservation(kBase, totalBytes);

		return false;
	}

	m_committedBytes += kStackBytes;

	outAllocation.stackBase = kStackBase;
	outAllocation.stackSize = kStackBytes;
	outAllocation.guardSize = kGuardBytes;

	return true;
}

void ThreadStackAllocator::release(const ThreadStackAllocation& allocation) noexcept {
	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	void* const kReservationBase = static_cast<char*>(allocation.stackBase) - allocation.guardSize;
	const std::size_t kReservationSize = allocation.stackSize + allocation.guardSize;

	memory::releaseReservation(kReservationBase, kReservationSize);
	m_committedBytes -= allocation.stackSize;
}

} // namespace trivial::thread

#else
#error "ThreadStackAllocator is POSIX only"

#endif // TRIVIAL_PLATFORM_POSIX

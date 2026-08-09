#include <trivial/core/memory/page_allocator.h>

#include <cstddef>
#include <cstdint>

#include <trivial/core/assert.h>
#include <trivial/core/log.h>
#include <trivial/core/platform.h>
#include <trivial/core/profile.h>

#if TRIVIAL_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#elif TRIVIAL_PLATFORM_POSIX
#include <cerrno>
#include <sys/mman.h>
#include <unistd.h>

#else
#error "Unsupported platform in page_allocator.cpp"

#endif // Platform check

namespace {

std::size_t queryOsPageSize() {
#if TRIVIAL_PLATFORM_WINDOWS
	SYSTEM_INFO info;
	GetSystemInfo(&info);

	return static_cast<std::size_t>(info.dwPageSize);

#elif TRIVIAL_PLATFORM_POSIX
	long result = sysconf(_SC_PAGESIZE);

	TRIVIAL_ASSERT(result > 0);

	return static_cast<std::size_t>(result);

#endif // Platform check
}

std::size_t roundUpToPageSize(std::size_t bytes, std::size_t pageSize) {
	TRIVIAL_ASSERT(pageSize > 0);
	TRIVIAL_ASSERT((pageSize & (pageSize - 1)) == 0);
	TRIVIAL_ASSERT(bytes <= SIZE_MAX - (pageSize - 1));

	return (bytes + pageSize - 1) & ~(pageSize - 1);
}

} // namespace

namespace trivial::memory {

bool PageAllocator::init(std::size_t reserveSize) noexcept {
	TRIVIAL_ASSERT(reserveSize > 0);

	bool needsOomReport = false;
	std::size_t oomRequestedSize = 0;
	const char* oomContext = nullptr;
	int oomErrorCode = 0;
	bool succeeded = false;

	{
		std::lock_guard<std::mutex> lock(m_stateMutex);

		TRIVIAL_ASSERT(m_base == nullptr);

		m_pageSize = queryOsPageSize();
		m_reservedSize = roundUpToPageSize(reserveSize, m_pageSize);

#if TRIVIAL_PLATFORM_WINDOWS
		m_base = VirtualAlloc(nullptr, m_reservedSize, MEM_RESERVE, PAGE_NOACCESS);
		if (m_base == nullptr) {
			oomErrorCode = static_cast<int>(GetLastError());
		}

#elif TRIVIAL_PLATFORM_POSIX
		void* mapResult = mmap(nullptr, m_reservedSize, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

		if (mapResult == MAP_FAILED) {
			m_base = nullptr;
			oomErrorCode = errno;
		} else {
			m_base = mapResult;
		}

#endif // Platform check

		if (m_base == nullptr) {
			needsOomReport = true;
			oomRequestedSize = reserveSize;
			oomContext = "PageAllocator::init reservation failed";
			m_reservedSize = 0;
		} else {
			m_usedBytes = 0;
			succeeded = true;
		}
	}

	if (needsOomReport) {
		reportOom(oomRequestedSize, oomContext, oomErrorCode);
	}

	return succeeded;
}

void PageAllocator::shutdown() noexcept {
	std::lock_guard<std::mutex> lock(m_stateMutex);

	if (m_base == nullptr) {
		return;
	}

#if TRIVIAL_PLATFORM_WINDOWS
	if (!VirtualFree(m_base, 0, MEM_RELEASE)) {
		TRIVIAL_LOG_ERROR_PREFIX("PageAllocator", "shutdown failed (VirtualFree)");
	}
#elif TRIVIAL_PLATFORM_POSIX
	if (munmap(m_base, m_reservedSize) != 0) {
		TRIVIAL_LOG_ERROR_PREFIX("PageAllocator", "shutdown failed (munmap)");
	}
#endif

	m_base = nullptr;
	m_reservedSize = 0;
	m_usedBytes = 0;
}

void* PageAllocator::getMoreMemory(std::size_t bytes) noexcept {
	TRIVIAL_PROFILE_FUNCTION();
	TRIVIAL_ASSERT(bytes > 0);
	TRIVIAL_ASSERT(bytes % m_pageSize == 0);

	bool needsOomReport = false;
	const char* oomContext = nullptr;
	int oomErrorCode = 0;
	void* result = nullptr;

	{
		std::lock_guard<std::mutex> lock(m_stateMutex);
		std::size_t newUsedBytes = m_usedBytes + bytes;

		if (newUsedBytes <= m_reservedSize) {
			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
			void* commitAddr = static_cast<char*>(m_base) + m_usedBytes;

#if TRIVIAL_PLATFORM_WINDOWS
			const void* kCommitResult = VirtualAlloc(commitAddr, bytes, MEM_COMMIT, PAGE_READWRITE);
			if (kCommitResult == nullptr) {
				needsOomReport = true;
				oomContext = "PageAllocator::getMoreMemory commit failed";
				oomErrorCode = static_cast<int>(GetLastError());
			} else {
				m_usedBytes = newUsedBytes;
				result = kCommitResult;
			}

#elif TRIVIAL_PLATFORM_POSIX
			const int kCommitResult = mprotect(commitAddr, bytes, PROT_READ | PROT_WRITE);

			if (kCommitResult != 0) {
				needsOomReport = true;
				oomContext = "PageAllocator::getMoreMemory commit failed";
				oomErrorCode = errno;
			} else {
				m_usedBytes = newUsedBytes;
				result = commitAddr;
			}

#endif // Platform check

#if TRIVIAL_ENABLE_MEMORY_DEBUG_STATS
			if (result != nullptr) {
				std::size_t currentCommitted = m_debugCommittedBytes.fetch_add(bytes) + bytes;
				std::size_t currentPeak = m_debugPeakCommittedBytes.load();
				if (currentCommitted > currentPeak) {
					m_debugPeakCommittedBytes.store(currentCommitted);
				}
			}
#endif // TRIVIAL_ENABLE_MEMORY_DEBUG_STATS
		} else {
			needsOomReport = true;
			oomContext = "PageAllocator::getMoreMemory exceeds reservation";
		}
	}

	if (needsOomReport) {
		reportOom(bytes, oomContext, oomErrorCode);
	}

	return result;
}

bool PageAllocator::decommit(void* addr, std::size_t bytes) const noexcept {
	TRIVIAL_PROFILE_FUNCTION();
	TRIVIAL_ASSERT(addr != nullptr);
	TRIVIAL_ASSERT(bytes > 0);
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	TRIVIAL_ASSERT(reinterpret_cast<std::uintptr_t>(addr) % m_pageSize == 0);
	TRIVIAL_ASSERT(bytes % m_pageSize == 0);

#if TRIVIAL_PLATFORM_WINDOWS
	if (!VirtualFree(addr, bytes, MEM_DECOMMIT)) {
		TRIVIAL_LOG_ERROR_PREFIX("PageAllocator", "decommit failed (VirtualFree)");
		return false;
	}
#elif TRIVIAL_PLATFORM_POSIX
	if (madvise(addr, bytes, MADV_DONTNEED) != 0) {
		TRIVIAL_LOG_ERROR_PREFIX("PageAllocator", "decommit failed (madvise)");
		return false;
	}
	if (mprotect(addr, bytes, PROT_NONE) != 0) {
		TRIVIAL_LOG_ERROR_PREFIX("PageAllocator", "decommit failed (mprotect)");
		return false;
	}
#endif

#if TRIVIAL_ENABLE_MEMORY_DEBUG_STATS
	m_debugCommittedBytes.fetch_sub(bytes);
#endif

	return true;
}
#if TRIVIAL_ENABLE_MEMORY_DEBUG_STATS
PageAllocator::Stats PageAllocator::getStats() const noexcept {
	std::lock_guard<std::mutex> lock(m_stateMutex);

	std::size_t committed = m_debugCommittedBytes.load();
	std::size_t peak = m_debugPeakCommittedBytes.load();
	std::size_t decommittedBytes = peak - committed;

	return Stats{.reservedBytes = m_reservedSize,
	             .usedBytes = m_usedBytes,
	             .committedBytes = committed,
	             .decommittedBytes = decommittedBytes,
	             .peakCommittedBytes = peak};
}
#endif // TRIVIAL_ENABLE_MEMORY_DEBUG_STATS

void PageAllocator::reportOom(std::size_t requestedSize, const char* context, int osErrorCode) noexcept {
	TRIVIAL_LOG_OOM_FAILURE("PageAllocator", context, requestedSize, osErrorCode);

	std::lock_guard<std::mutex> lock(m_oomMutex);

	if (m_oomHandler != nullptr) {
		OomInfo info{.requestedSize = requestedSize, .context = context, .osErrorCode = osErrorCode};
		m_oomHandler(info);
	}
}

} // namespace trivial::memory

#ifndef TRIVIAL_CORE_MEMORY_PAGE_ALLOCATOR_H
#define TRIVIAL_CORE_MEMORY_PAGE_ALLOCATOR_H

#include <atomic>
#include <cstddef>
#include <mutex>

#include <trivial/core/config.h>
#include <trivial/core/memory/oom_handler.h>

// TODO: Make a general memory config file
#if TRIVIAL_CONFIG_DEBUG
#define TRIVIAL_ENABLE_MEMORY_DEBUG_STATS 1
#endif // TRIVIAL_CONFIG_DEBUG

namespace trivial::memory {

// Destruction whilst multiple threads exist will result in undefined and more
// importantly unsafe behaviour on the chance that there is a race between
// destruction and interaction - non-issue unless creating threads outside of
// the engine
class PageAllocator {
public:
#if TRIVIAL_ENABLE_MEMORY_DEBUG_STATS
	struct Stats {
		std::size_t reservedBytes = 0;
		std::size_t usedBytes = 0;
		std::size_t committedBytes = 0;
		std::size_t decommittedBytes = 0;
		std::size_t peakCommittedBytes = 0;
	};
#endif // TRIVIAL_ENABLE_MEMORY_DEBUG_STATS

	PageAllocator() noexcept = default;
	~PageAllocator() { shutdown(); };

	PageAllocator(const PageAllocator&) = delete;
	PageAllocator& operator=(const PageAllocator&) = delete;

	PageAllocator(PageAllocator&&) = delete;
	PageAllocator& operator=(PageAllocator&&) = delete;

	bool init(std::size_t reserveSize) noexcept;
	void shutdown() noexcept;

	void* getMoreMemory(std::size_t bytes) noexcept;
	bool decommit(void* addr, std::size_t bytes) const noexcept;

	std::size_t pageSize() const noexcept { return m_pageSize; }

#if TRIVIAL_ENABLE_MEMORY_DEBUG_STATS
	Stats getStats() const noexcept;
#endif

	void setOomHandler(OomHandler handler) noexcept { m_oomHandler = handler; }

private:
	void reportOom(std::size_t requestedSize, const char* context, int osErrorCode) noexcept;

	mutable std::mutex m_stateMutex;
	std::mutex m_oomMutex;

	void* m_base = nullptr;
	std::size_t m_reservedSize = 0;
	std::size_t m_usedBytes = 0;
	std::size_t m_pageSize = 0;

	OomHandler m_oomHandler = nullptr;

#if TRIVIAL_ENABLE_MEMORY_DEBUG_STATS
	// Probably bad practice but makes release cleaner
	mutable std::atomic<std::size_t> m_debugCommittedBytes{0};
	mutable std::atomic<std::size_t> m_debugPeakCommittedBytes{0};
#endif
};

} // namespace trivial::memory

#endif // TRIVIAL_CORE_MEMORY_PAGE_ALLOCATOR_H

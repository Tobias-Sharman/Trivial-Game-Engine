#ifndef TRIVIAL_CORE_SYNC_SEMAPHORE_H
#define TRIVIAL_CORE_SYNC_SEMAPHORE_H

#include <atomic>
#include <cstddef>

#include <trivial/core/compiler.h>

namespace trivial::sync {

class Semaphore {
public:
	explicit Semaphore(std::size_t initialCount) noexcept
	    : m_count(initialCount) {}

	~Semaphore() noexcept = default;

	Semaphore(const Semaphore&) = delete;
	Semaphore& operator=(const Semaphore&) = delete;

	Semaphore(Semaphore&&) = delete;
	Semaphore& operator=(Semaphore&&) = delete;

	[[nodiscard]] TRIVIAL_FORCE_INLINE bool tryAcquire() noexcept {
		std::size_t count = m_count.load(std::memory_order_relaxed);

		while (count > 0) {
			if (m_count.compare_exchange_weak(count, count - 1, std::memory_order_acquire, std::memory_order_relaxed)) {
				return true;
			}
		}

		return false;
	}

	void acquire() noexcept;
	void release() noexcept;

private:
	std::atomic<std::size_t> m_count;
};

} // namespace trivial::sync

#endif // TRIVIAL_CORE_SYNC_SEMAPHORE_H

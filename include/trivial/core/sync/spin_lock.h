#ifndef TRIVIAL_CORE_SYNC_SPIN_LOCK_H
#define TRIVIAL_CORE_SYNC_SPIN_LOCK_H

#include <atomic>

#include <trivial/core/sync/spin_wait.h>

namespace trivial::sync {

class SpinLock {
public:
	SpinLock() noexcept = default;

	~SpinLock() noexcept = default;

	SpinLock(const SpinLock&) = delete;
	SpinLock& operator=(const SpinLock&) = delete;

	SpinLock(SpinLock&&) = delete;
	SpinLock& operator=(SpinLock&&) = delete;

	[[nodiscard]] TRIVIAL_FORCE_INLINE bool tryLock() noexcept {
		return !m_state.exchange(true, std::memory_order_acquire);
	}

	TRIVIAL_FORCE_INLINE void lock() noexcept {
		std::uint32_t spinCount = 0;

		while (!tryLock()) {
			spinWaitForever(spinCount);
		}
	}

	TRIVIAL_FORCE_INLINE void unlock() noexcept {
		m_state.store(false, std::memory_order_release);
	}

private:
	std::atomic<bool> m_state{false};
};

} // namespace trivial::sync

#endif // TRIVIAL_CORE_SYNC_SPIN_LOCK_H

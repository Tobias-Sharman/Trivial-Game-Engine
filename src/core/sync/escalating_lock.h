#ifndef TRIVIAL_SRC_CORE_SYNC_ESCALATING_LOCK_H
#define TRIVIAL_SRC_CORE_SYNC_ESCALATING_LOCK_H

#include <atomic>
#include <cstdint>

#include <trivial/core/assert.h>
#include <trivial/core/compiler.h>

namespace trivial::sync {

class EscalatingLock {
public:
	EscalatingLock() noexcept = default;

	~EscalatingLock() noexcept {
		TRIVIAL_ASSERT((m_state.load(std::memory_order_relaxed) & (kLockedBit | kQueueMask)) == 0);
	}

	EscalatingLock(const EscalatingLock&) = delete;
	EscalatingLock& operator=(const EscalatingLock&) = delete;

	EscalatingLock(EscalatingLock&&) = delete;
	EscalatingLock& operator=(EscalatingLock&&) = delete;

	TRIVIAL_FORCE_INLINE void lock() noexcept {
		std::uintptr_t expected = 0;
		if (m_state.compare_exchange_weak(expected, kLockedBit, std::memory_order_acquire, std::memory_order_relaxed)) {
			return;
		}

		lockSlow();
	}

	TRIVIAL_FORCE_INLINE void unlock() noexcept {
		const std::uintptr_t kPreviousState = m_state.fetch_sub(kLockedBit, std::memory_order_release);
		TRIVIAL_ASSERT((kPreviousState & kLockedBit) != 0);

		if ((kPreviousState & kQueueLockedBit) != 0 || (kPreviousState & kQueueMask) == 0) {
			return;
		}

		unlockSlow();
	}

private:
	TRIVIAL_COLD void lockSlow() noexcept;
	TRIVIAL_COLD void unlockSlow() noexcept;

	static constexpr std::uintptr_t kLockedBit = 1;
	static constexpr std::uintptr_t kQueueLockedBit = 2;
	static constexpr std::uintptr_t kQueueMask = ~static_cast<std::uintptr_t>(3);

	std::atomic<std::uintptr_t> m_state{0};
};

} // namespace trivial::sync

#endif // TRIVIAL_SRC_CORE_SYNC_ESCALATING_LOCK_H

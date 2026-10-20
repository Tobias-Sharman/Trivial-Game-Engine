#ifndef TRIVIAL_SRC_CORE_SYNC_ESCALATING_LOCK_H
#define TRIVIAL_SRC_CORE_SYNC_ESCALATING_LOCK_H

#include <atomic>
#include <cstdint>

#include <trivial/core/assert.h>
#include <trivial/core/compiler.h>

#define TRIVIAL_SYNC_ESCALATING_LOCK_LOCKED_BIT (std::uintptr_t{1})
#define TRIVIAL_SYNC_ESCALATING_LOCK_QUEUE_LOCKED_BIT (std::uintptr_t{2})
#define TRIVIAL_SYNC_ESCALATING_LOCK_QUEUE_MASK (~std::uintptr_t{3})

namespace trivial::sync {

class EscalatingLock {
public:
	EscalatingLock() noexcept = default;

	~EscalatingLock() noexcept {
		TRIVIAL_ASSERT((m_state.load(std::memory_order_relaxed)
		                & (TRIVIAL_SYNC_ESCALATING_LOCK_LOCKED_BIT | TRIVIAL_SYNC_ESCALATING_LOCK_QUEUE_MASK))
		               == 0);
	}

	EscalatingLock(const EscalatingLock&) = delete;
	EscalatingLock& operator=(const EscalatingLock&) = delete;

	EscalatingLock(EscalatingLock&&) = delete;
	EscalatingLock& operator=(EscalatingLock&&) = delete;

	TRIVIAL_FORCE_INLINE void lock() noexcept {
		std::uintptr_t expected = 0;
		if (m_state.compare_exchange_weak(expected,
		                                  TRIVIAL_SYNC_ESCALATING_LOCK_LOCKED_BIT,
		                                  std::memory_order_acquire,
		                                  std::memory_order_relaxed)) {
			return;
		}

		lockSlow();
	}

	TRIVIAL_FORCE_INLINE void unlock() noexcept {
		const std::uintptr_t kPreviousState
		    = m_state.fetch_sub(TRIVIAL_SYNC_ESCALATING_LOCK_LOCKED_BIT, std::memory_order_release);
		TRIVIAL_ASSERT((kPreviousState & TRIVIAL_SYNC_ESCALATING_LOCK_LOCKED_BIT) != 0);

		if ((kPreviousState & TRIVIAL_SYNC_ESCALATING_LOCK_QUEUE_LOCKED_BIT) != 0
		    || (kPreviousState & TRIVIAL_SYNC_ESCALATING_LOCK_QUEUE_MASK) == 0) {
			return;
		}

		unlockSlow();
	}

private:
	TRIVIAL_COLD void lockSlow() noexcept;
	TRIVIAL_COLD void unlockSlow() noexcept;

	std::atomic<std::uintptr_t> m_state{0};
};

} // namespace trivial::sync

#ifndef TRIVIAL_SYNC_ESCALATING_LOCK_IMPLEMENTATION
#undef TRIVIAL_SYNC_ESCALATING_LOCK_LOCKED_BIT
#undef TRIVIAL_SYNC_ESCALATING_LOCK_QUEUE_LOCKED_BIT
#undef TRIVIAL_SYNC_ESCALATING_LOCK_QUEUE_MASK
#endif // TRIVIAL_SYNC_ESCALATING_LOCK_IMPLEMENTATION

#endif // TRIVIAL_SRC_CORE_SYNC_ESCALATING_LOCK_H

#ifndef TRIVIAL_CORE_SYNC_MUTEX_H
#define TRIVIAL_CORE_SYNC_MUTEX_H

#include <atomic>
#include <cstdint>

#include <trivial/core/compiler.h>

// TODO: Fairness deferred until a timer-free design can be figured out

#define TRIVIAL_SYNC_MUTEX_LOCKED_BIT (std::uint8_t{0b01})
#define TRIVIAL_SYNC_MUTEX_PARKED_BIT (std::uint8_t{0b10})

namespace trivial::sync {

class Mutex {
public:
	Mutex() noexcept = default;

	~Mutex() noexcept = default;

	Mutex(const Mutex&) = delete;
	Mutex& operator=(const Mutex&) = delete;

	Mutex(Mutex&&) = delete;
	Mutex& operator=(Mutex&&) = delete;

	TRIVIAL_FORCE_INLINE void lock() noexcept {
		std::uint8_t expected = 0;
		if (m_state.compare_exchange_weak(expected,
		                                  TRIVIAL_SYNC_MUTEX_LOCKED_BIT,
		                                  std::memory_order_acquire,
		                                  std::memory_order_relaxed)) {
			return;
		}

		lockSlow();
	}

	TRIVIAL_FORCE_INLINE void unlock() noexcept {
		std::uint8_t expected = TRIVIAL_SYNC_MUTEX_LOCKED_BIT;
		if (m_state.compare_exchange_strong(expected, 0, std::memory_order_release, std::memory_order_relaxed)) {
			return;
		}

		unlockSlow();
	}

	// For condition variable not general use
	[[nodiscard]] TRIVIAL_FORCE_INLINE bool markParkedIfLocked() noexcept {
		std::uint8_t state = m_state.load(std::memory_order_relaxed);

		for (;;) {
			if ((state & TRIVIAL_SYNC_MUTEX_LOCKED_BIT) == 0) {
				return false;
			}

			if (m_state.compare_exchange_weak(state,
			                                  static_cast<std::uint8_t>(state | TRIVIAL_SYNC_MUTEX_PARKED_BIT),
			                                  std::memory_order_relaxed,
			                                  std::memory_order_relaxed)) {
				return true;
			}
		}
	}

	TRIVIAL_FORCE_INLINE void markParked() noexcept {
		m_state.fetch_or(TRIVIAL_SYNC_MUTEX_PARKED_BIT, std::memory_order_relaxed);
	}

private:
	TRIVIAL_COLD void lockSlow() noexcept;
	TRIVIAL_COLD void unlockSlow() noexcept;

	std::atomic<std::uint8_t> m_state{0};
};

} // namespace trivial::sync

#ifndef TRIVIAL_SYNC_MUTEX_IMPLEMENTATION
#undef TRIVIAL_SYNC_MUTEX_LOCKED_BIT
#undef TRIVIAL_SYNC_MUTEX_PARKED_BIT
#endif // TRIVIAL_SYNC_MUTEX_IMPLEMENTATION

#endif // TRIVIAL_CORE_SYNC_MUTEX_H

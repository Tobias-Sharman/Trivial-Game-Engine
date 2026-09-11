#ifndef TRIVIAL_CORE_SYNC_MUTEX_H
#define TRIVIAL_CORE_SYNC_MUTEX_H

#include <atomic>
#include <cstdint>

#include <trivial/core/compiler.h>

// TODO: Fairness deferred until a timer-free design can be figured out

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
		if (m_state.compare_exchange_weak(expected, kLockedBit, std::memory_order_acquire, std::memory_order_relaxed)) {
			return;
		}

		lockSlow();
	}

	TRIVIAL_FORCE_INLINE void unlock() noexcept {
		std::uint8_t expected = kLockedBit;
		if (m_state.compare_exchange_strong(expected, 0, std::memory_order_release, std::memory_order_relaxed)) {
			return;
		}

		unlockSlow();
	}

private:
	TRIVIAL_COLD void lockSlow() noexcept;
	TRIVIAL_COLD void unlockSlow() noexcept;

	static constexpr std::uint8_t kLockedBit = 0b01;
	static constexpr std::uint8_t kParkedBit = 0b10;

	std::atomic<std::uint8_t> m_state{0};
};

} // namespace trivial::sync

#endif // TRIVIAL_CORE_SYNC_MUTEX_H

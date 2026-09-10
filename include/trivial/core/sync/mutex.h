#ifndef TRIVIAL_CORE_SYNC_MUTEX_H
#define TRIVIAL_CORE_SYNC_MUTEX_H

#include <atomic>
#include <cstdint>

#include <trivial/core/compiler.h>

// TODO: When implementing hash table backed parking lot update mutex to have
//       fair unparking

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
		std::uint8_t expected = kUnlocked;
		if (m_state.compare_exchange_strong(expected,
		                                    kLockedUncontended,
		                                    std::memory_order_acquire,
		                                    std::memory_order_relaxed)) {
			return;
		}

		lockSlow();
	}

	TRIVIAL_FORCE_INLINE void unlock() noexcept {
		if (m_state.fetch_sub(1, std::memory_order_release) != kLockedUncontended) {
			unlockSlow();
		}
	}

private:
	TRIVIAL_COLD void lockSlow() noexcept;
	TRIVIAL_COLD void unlockSlow() noexcept;

	static constexpr std::uint8_t kUnlocked = 0;
	static constexpr std::uint8_t kLockedUncontended = 1;
	static constexpr std::uint8_t kLockedContended = 2;

	std::atomic<std::uint8_t> m_state{kUnlocked};
};

} // namespace trivial::sync

#endif // TRIVIAL_CORE_SYNC_MUTEX_H

#ifndef TRIVIAL_CORE_SYNC_LOCK_GUARD_H
#define TRIVIAL_CORE_SYNC_LOCK_GUARD_H

#include <trivial/core/compiler.h>

namespace trivial::sync {

template <typename Lock>
class LockGuard {
public:
	[[nodiscard]] TRIVIAL_FORCE_INLINE explicit LockGuard(Lock& lock) noexcept
	    : m_lock(lock) {
		m_lock.lock();
	}

	TRIVIAL_FORCE_INLINE ~LockGuard() noexcept { m_lock.unlock(); }

	LockGuard(const LockGuard&) = delete;
	LockGuard& operator=(const LockGuard&) = delete;

	LockGuard(LockGuard&&) = delete;
	LockGuard& operator=(LockGuard&&) = delete;

	TRIVIAL_FORCE_INLINE void lock() noexcept { m_lock.lock(); }

	TRIVIAL_FORCE_INLINE void unlock() noexcept { m_lock.unlock(); }

private:
	Lock& m_lock;
};

} // namespace trivial::sync

#endif // TRIVIAL_CORE_SYNC_LOCK_GUARD_H

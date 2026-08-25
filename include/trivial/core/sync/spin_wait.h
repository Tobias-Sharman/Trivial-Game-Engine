#ifndef TRIVIAL_CORE_SYNC_SPIN_WAIT_H
#define TRIVIAL_CORE_SYNC_SPIN_WAIT_H

#include <cstdint>

#include <trivial/core/cpu_hints.h>
#include <trivial/core/sync/sync_config.h>
#include <trivial/core/thread/thread.h>

namespace trivial::sync {

[[nodiscard]] TRIVIAL_FORCE_INLINE bool spinWaitToLimit(std::uint32_t& spinCount) noexcept {
	if (spinCount >= TRIVIAL_SYNC_MAX_SPIN_COUNT) {
		return false;
	}

	++spinCount;

	if (spinCount <= TRIVIAL_SYNC_SPIN_COUNT_BEFORE_YIELD) {
		const std::uint32_t kPauseIterations = TRIVIAL_SYNC_MIN_PAUSE_ITERATIONS << (spinCount - 1);

		for (std::uint32_t i = 0; i < kPauseIterations; ++i) {
			TRIVIAL_CPU_PAUSE();
		}
	} else {
		trivial::thread::Thread::yield();
	}

	return true;
}

TRIVIAL_FORCE_INLINE void spinWaitForever(std::uint32_t& spinCount) noexcept {
	++spinCount;

	if (spinCount <= TRIVIAL_SYNC_SPIN_COUNT_BEFORE_YIELD) {
		const std::uint32_t kPauseIterations = TRIVIAL_SYNC_MIN_PAUSE_ITERATIONS << (spinCount - 1);

		for (std::uint32_t i = 0; i < kPauseIterations; ++i) {
			TRIVIAL_CPU_PAUSE();
		}
	} else {
		trivial::thread::Thread::yield();
	}
}

TRIVIAL_FORCE_INLINE void spinWaitForeverNoYield(std::uint32_t& spinCount) noexcept {
	if (spinCount < TRIVIAL_SYNC_MAX_SPIN_COUNT) {
		++spinCount;
	}

	const std::uint32_t kPauseIterations = TRIVIAL_SYNC_MIN_PAUSE_ITERATIONS << (spinCount - 1);

	for (std::uint32_t i = 0; i < kPauseIterations; ++i) {
		TRIVIAL_CPU_PAUSE();
	}
}

} // namespace trivial::sync

#endif // TRIVIAL_CORE_SYNC_SPIN_WAIT_H

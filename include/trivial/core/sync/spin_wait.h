#ifndef TRIVIAL_CORE_SYNC_SPIN_WAIT_H
#define TRIVIAL_CORE_SYNC_SPIN_WAIT_H

#include <cstdint>
#include <thread>

#include <trivial/core/cpu_hints.h>

namespace trivial::sync {

// Values from spinwait.rs in the parking_lot_core crate - checked 20-08-2026
inline constexpr std::uint32_t g_kSpinCountBeforeYield = 3;
inline constexpr std::uint32_t g_kMaxSpinCount = 10;
inline constexpr std::uint32_t g_kMinPauseIterations = 2;
inline constexpr std::uint32_t g_kMaxPauseIterations = g_kMinPauseIterations << (g_kSpinCountBeforeYield - 1);

[[nodiscard]] TRIVIAL_FORCE_INLINE bool spinWaitToLimit(std::uint32_t& spinCount) noexcept {
	if (spinCount >= g_kMaxSpinCount) {
		return false;
	}

	++spinCount;

	if (spinCount <= g_kSpinCountBeforeYield) {
		const std::uint32_t kPauseIterations = g_kMinPauseIterations << (spinCount - 1);

		for (std::uint32_t i = 0; i < kPauseIterations; ++i) {
			TRIVIAL_CPU_PAUSE();
		}
	} else {
		std::this_thread::yield();
	}

	return true;
}

TRIVIAL_FORCE_INLINE void spinWaitForever(std::uint32_t& spinCount) noexcept {
	++spinCount;

	if (spinCount <= g_kSpinCountBeforeYield) {
		const std::uint32_t kPauseIterations = g_kMinPauseIterations << (spinCount - 1);

		for (std::uint32_t i = 0; i < kPauseIterations; ++i) {
			TRIVIAL_CPU_PAUSE();
		}
	} else {
		std::this_thread::yield();
	}
}

TRIVIAL_FORCE_INLINE void spinWaitForeverNoYield(std::uint32_t& spinCount) noexcept {
	if (spinCount < g_kMaxSpinCount) {
		++spinCount;
	}

	const std::uint32_t kPauseIterations = g_kMinPauseIterations << (spinCount - 1);

	for (std::uint32_t i = 0; i < kPauseIterations; ++i) {
		TRIVIAL_CPU_PAUSE();
	}
}

} // namespace trivial::sync

#endif // TRIVIAL_CORE_SYNC_SPIN_WAIT_H

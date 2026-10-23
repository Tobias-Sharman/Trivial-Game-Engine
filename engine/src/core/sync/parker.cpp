#include "core/sync/parker.h"

#include <atomic>

namespace trivial::sync {

void Parker::prepare() noexcept {
	m_state.store(1, std::memory_order_relaxed);
}

void Parker::commitUnpark() noexcept {
	m_state.store(0, std::memory_order_release);
}

[[nodiscard]] bool Parker::timedOut() noexcept {
	return m_state.load(std::memory_order_relaxed) != 0;
}

} // namespace trivial::sync

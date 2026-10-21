#include "core/sync/parker.h"

#include <atomic>

namespace trivial::sync {

void Parker::prepare() noexcept {
	m_state.state.store(1, std::memory_order_relaxed);
}

[[nodiscard]] UnparkHandle Parker::beginUnpark() noexcept {
	m_state.state.store(0, std::memory_order_release);
	return UnparkHandle(&m_state);
}

[[nodiscard]] bool Parker::timedOut() noexcept {
	return m_state.state.load(std::memory_order_relaxed) != 0;
}

} // namespace trivial::sync

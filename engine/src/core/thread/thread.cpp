#include <trivial/core/thread/thread.h>

#include <atomic>
#include <cstdlib>

#include <trivial/core/assert.h>
#include <trivial/core/log.h>

namespace trivial::thread {

Thread::~Thread() noexcept {
	if (joinable()) [[unlikely]] {
		TRIVIAL_LOG_FATAL_PREFIX("Thread", "destroyed while still joinable - call join() first");
		std::abort();
	}

	if (detail::g_currentThread == this) {
		detail::g_currentThread = nullptr;
	}
}

void Thread::resume() noexcept {
	TRIVIAL_ASSERT(m_state.load(std::memory_order_relaxed) == ThreadState::Suspended);

	m_state.store(ThreadState::Running, std::memory_order_release);
	m_state.notify_one();
}

} // namespace trivial::thread

#ifndef TRIVIAL_CORE_SYNC_EVENT_H
#define TRIVIAL_CORE_SYNC_EVENT_H

#include <atomic>
#include <chrono>

namespace trivial::sync {

class Event {
public:
	Event() noexcept = default;

	~Event() noexcept = default;

	Event(const Event&) = delete;
	Event& operator=(const Event&) = delete;

	Event(Event&&) = delete;
	Event& operator=(Event&&) = delete;

	[[nodiscard]] bool isTriggered() const noexcept { return m_isTriggered.load(std::memory_order_acquire); }

	void wait() noexcept;
	[[nodiscard]] bool waitFor(std::chrono::nanoseconds timeout) noexcept;

	void trigger() noexcept;

	void reset() noexcept { m_isTriggered.store(false, std::memory_order_relaxed); }

private:
	std::atomic<bool> m_isTriggered{false};
};

} // namespace trivial::sync

#endif // TRIVIAL_CORE_SYNC_EVENT_H

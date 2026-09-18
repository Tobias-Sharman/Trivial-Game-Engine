#ifndef TRIVIAL_CORE_SYNC_LATCH_H
#define TRIVIAL_CORE_SYNC_LATCH_H

#include <atomic>
#include <cstddef>

namespace trivial::sync {

class Latch {
public:
	explicit Latch(std::size_t count) noexcept
	    : m_remaining(count) {}

	~Latch() noexcept = default;

	Latch(const Latch&) = delete;
	Latch& operator=(const Latch&) = delete;

	Latch(Latch&&) = delete;
	Latch& operator=(Latch&&) = delete;

	[[nodiscard]] std::size_t remaining() const noexcept { return m_remaining.load(std::memory_order_acquire); }

	void wait() noexcept;

	void countDown() noexcept;

private:
	std::atomic<std::size_t> m_remaining;
};

} // namespace trivial::sync

#endif // TRIVIAL_CORE_SYNC_LATCH_H

#ifndef TRIVIAL_SRC_CORE_SYNC_PARKER_H
#define TRIVIAL_SRC_CORE_SYNC_PARKER_H

#include <atomic>
#include <cstdint>

#include <trivial/core/time/duration.h>

namespace trivial::sync {

struct ParkState {
	std::atomic<std::uint32_t> state{0};
};

class UnparkHandle {
public:
	explicit UnparkHandle(ParkState* state) noexcept
	    : m_state(state) {}

	void wake() noexcept;

private:
	ParkState* m_state;
};

class Parker {
public:
	Parker() noexcept = default;

	~Parker() noexcept = default;

	Parker(const Parker&) = delete;
	Parker& operator=(const Parker&) = delete;

	Parker(Parker&&) = delete;
	Parker& operator=(Parker&&) = delete;

	void prepare() noexcept;

	void park() noexcept;

	[[nodiscard]] bool parkFor(time::Duration timeout) noexcept;

	[[nodiscard]] UnparkHandle beginUnpark() noexcept;

	[[nodiscard]] bool timedOut() noexcept;

private:
	ParkState m_state;
};

} // namespace trivial::sync

#endif // TRIVIAL_SRC_CORE_SYNC_PARKER_H

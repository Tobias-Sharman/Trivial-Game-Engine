#ifndef TRIVIAL_SRC_CORE_SYNC_PARKER_H
#define TRIVIAL_SRC_CORE_SYNC_PARKER_H

#include <chrono>

#include <trivial/core/platform.h>

#if TRIVIAL_PLATFORM_WINDOWS || TRIVIAL_PLATFORM_LINUX
#include <atomic>
#include <cstdint>

#elif TRIVIAL_PLATFORM_MACOS
#include <pthread.h>

#endif // Platform-specific headers

namespace trivial::sync {

#if TRIVIAL_PLATFORM_WINDOWS || TRIVIAL_PLATFORM_LINUX

struct ParkState {
	std::atomic<std::uint32_t> state{0};
};

#elif TRIVIAL_PLATFORM_MACOS

struct ParkState {
	pthread_mutex_t mutex{};
	pthread_cond_t condvar{};
	bool notified = false;
};

#endif // Platform-specific ParkState

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
	Parker() noexcept;

	~Parker() noexcept;

	Parker(const Parker&) = delete;
	Parker& operator=(const Parker&) = delete;

	Parker(Parker&&) = delete;
	Parker& operator=(Parker&&) = delete;

	void prepare() noexcept;

	void park() noexcept;

	[[nodiscard]] bool parkFor(std::chrono::nanoseconds lifetime) noexcept;

	[[nodiscard]] UnparkHandle beginUnpark() noexcept;

	[[nodiscard]] bool timedOut() noexcept;

private:
	ParkState m_state;
};

} // namespace trivial::sync

#endif // TRIVIAL_SRC_CORE_SYNC_PARKER_H

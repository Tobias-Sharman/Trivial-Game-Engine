#ifndef TRIVIAL_SRC_CORE_SYNC_PARKER_H
#define TRIVIAL_SRC_CORE_SYNC_PARKER_H

#include <atomic>
#include <cstdint>

#include <trivial/core/time/duration.h>

namespace trivial::sync {

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

	void commitUnpark() noexcept;

	void wake() noexcept;

	[[nodiscard]] bool timedOut() noexcept;

private:
	std::atomic<std::uint32_t> m_state{0};
};

} // namespace trivial::sync

#endif // TRIVIAL_SRC_CORE_SYNC_PARKER_H

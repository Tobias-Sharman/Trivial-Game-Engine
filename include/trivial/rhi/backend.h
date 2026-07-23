#ifndef TRIVIAL_RHI_BACKEND_H
#define TRIVIAL_RHI_BACKEND_H

#include <trivial/core/graphics_api.h>

namespace trivial::rhi {

class Backend {
public:
	Backend() = default;

	virtual ~Backend();

	Backend(const Backend&) = delete;
	Backend& operator=(const Backend&) = delete;

	Backend(Backend&&) = delete;
	Backend& operator=(Backend&&) = delete;

	[[nodiscard]] virtual GraphicsApi graphicsApi() const noexcept = 0;

	virtual void waitIdle() noexcept = 0;
};

} // namespace trivial::rhi

#endif // TRIVIAL_RHI_BACKEND_H

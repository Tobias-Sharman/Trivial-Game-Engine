#ifndef TRIVIAL_INTERNAL_RHI_BACKEND_H
#define TRIVIAL_INTERNAL_RHI_BACKEND_H

#include <trivial/core/graphics_api.h>

namespace trivial::internal::rhi {

class Backend {
public:
	Backend() = default;

	virtual ~Backend();

	Backend(const Backend&) = delete;
	Backend& operator=(const Backend&) = delete;

	Backend(Backend&&) = delete;
	Backend& operator=(Backend&&) = delete;

	[[nodiscard]] virtual GraphicsApi graphicsApi() const = 0;

	virtual void waitIdle() = 0;
};

} // namespace trivial::internal::rhi

#endif // TRIVIAL_INTERNAL_RHI_BACKEND_H

#ifndef TRIVIAL_INTERNAL_GPU_CONTEXT_H
#define TRIVIAL_INTERNAL_GPU_CONTEXT_H

#include <memory>

#include <trivial/engine_config.h>
#include <trivial/internal/platform/window.h>
#include <trivial/internal/rhi/backend.h>

namespace trivial::internal::gpu {

class Context {
public:
	Context() = delete;
	explicit Context(const EngineConfig* config, platform::Window* window);

	~Context();

	Context(const Context&) = delete;
	Context& operator=(const Context&) = delete;

	Context(Context&&) = delete;
	Context& operator=(Context&&) = delete;

	[[nodiscard]] GraphicsApi activeGraphicsApi() const { return m_backend->graphicsApi(); }

	void waitIdle();

private:
	std::unique_ptr<rhi::Backend> m_backend;
};

} // namespace trivial::internal::gpu

#endif // TRIVIAL_INTERNAL_GPU_CONTEXT_H

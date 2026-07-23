#ifndef TRIVIAL_GPU_CONTEXT_H
#define TRIVIAL_GPU_CONTEXT_H

#include <memory>

#include <trivial/engine_config.h>
#include <trivial/platform/window.h>
#include <trivial/rhi/backend.h>

namespace trivial::gpu {

class Context {
public:
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

} // namespace trivial::gpu

#endif // TRIVIAL_GPU_CONTEXT_H

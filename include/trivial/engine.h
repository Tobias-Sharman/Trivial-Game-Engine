#ifndef TRIVIAL_ENGINE_H
#define TRIVIAL_ENGINE_H

#include <trivial/application.h>
#include <trivial/engine_config.h>
#include <trivial/core/graphics_api.h>
#include <trivial/internal/gpu/context.h>
#include <trivial/internal/platform/window.h>
#include <trivial/render/renderer.h>
#include <trivial/time/engine_time.h>

namespace trivial {

class Engine {
public:
	Engine() = delete;
	explicit Engine(const EngineConfig* config);

	~Engine();

	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;

	Engine(Engine&&) = delete;
	Engine& operator=(Engine&&) = delete;

	void tick(Application& application);
	void run(Application& application);

	[[nodiscard]] GraphicsApi requestedGraphicsApi() const { return m_requestedGraphicsApi; }
	[[nodiscard]] GraphicsApi activeGraphicsApi() const { return m_gpu.activeGraphicsApi(); }

	[[nodiscard]] render::Renderer& renderer() { return m_renderer; }
	[[nodiscard]] const render::Renderer& renderer() const { return m_renderer; }

private:
	// TODO: Once form of what objects engine actually owns is explicit then update this
	GraphicsApi m_requestedGraphicsApi = GraphicsApi::Auto;
	EngineTime m_time;
	std::uint64_t m_frameIndex;

	internal::platform::Window m_window;
	internal::gpu::Context m_gpu;
	render::Renderer m_renderer;
};

} // namespace trivial

#endif // TRIVIAL_ENGINE_H

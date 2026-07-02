#ifndef TRIVIAL_ENGINE_H
#define TRIVIAL_ENGINE_H

#include <trivial/application.h>
#include <trivial/engine_config.h>
#include <trivial/core/graphics_api.h>
#include <trivial/internal/gpu/context.h>
#include <trivial/internal/platform/window.h>
#include <trivial/render/renderer.h>
#include <trivial/scene/scene.h>
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

	void run(Application& application);

	[[nodiscard]] GraphicsApi requestedGraphicsApi() const { return m_requestedGraphicsApi; }
	[[nodiscard]] GraphicsApi activeGraphicsApi() const { return m_gpu.activeGraphicsApi(); }

	[[nodiscard]] render::Renderer& renderer() { return m_renderer; }
	[[nodiscard]] const render::Renderer& renderer() const { return m_renderer; }

	[[nodiscard]] Scene& scene() { return m_scene; }
	[[nodiscard]] const Scene& scene() const { return m_scene; }

private:
	GraphicsApi m_requestedGraphicsApi = GraphicsApi::Auto;
	EngineTime m_time;

	internal::platform::Window m_window;
	internal::gpu::Context m_gpu;
	render::Renderer m_renderer;

	Scene m_scene;

	// NOTE: Remeber not all cache lines are 64 bytes, will want some special handling for 128 byte for m series chips
	//       Do not care about any server cpu that may have more
	//       When adding in support for gpu compute will need to take care for their different cache size too
};

} // namespace trivial

#endif // TRIVIAL_ENGINE_H

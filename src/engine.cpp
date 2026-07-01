#include <trivial/engine.h>

#include "core/assert.h"
#include "core/profile.h"

namespace trivial {

namespace {

GraphicsApi readRequestedGraphicsApi(const EngineConfig* config) {
	TRIVIAL_ASSERT(config != nullptr);

	return config->graphicsApi;
}

} // namespace

Engine::Engine(const EngineConfig* config)
    : m_requestedGraphicsApi(readRequestedGraphicsApi(config))
    , m_window(config)
    , m_gpu(config, &m_window)
    , m_renderer(&m_gpu) {
} // TODO: Add scene into here when having not just one scene

Engine::~Engine() = default; // Will later handle waiting for threads and memory freeing

void Engine::run() {
	TRIVIAL_PROFILE_THREAD("Main Thread");

	while (!m_window.shouldClose()) {
		TRIVIAL_PROFILE_FRAME("Frame");

		internal::platform::Window::pollEvents();

		ecs::World& world = m_scene.world();

		{
			TRIVIAL_PROFILE_SCOPE("Scene Update");

			for (int i = 0; i < 1000; ++i) {
				const ecs::Entity kEntity = world.create();
				world.destroy(kEntity);
			}
		}

		// Later begin and end frame stuff here too ig
		// Also maybe a render scope for profiling, see when having multithreading
	}
}

} // namespace trivial

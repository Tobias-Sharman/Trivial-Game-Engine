#include <trivial/engine.h>

#include <trivial/core/assert.h>
#include <trivial/core/config.h>
#include <trivial/core/profile.h>

namespace trivial {

namespace {

GraphicsApi readRequestedGraphicsApi(const EngineConfig* config) {
	TRIVIAL_ASSERT(config != nullptr);

	return config->graphicsApi;
}

} // namespace

Engine::Engine(const EngineConfig* config)
    : m_requestedGraphicsApi(readRequestedGraphicsApi(config))
    , m_frameIndex(0)
    , m_window(config)
    , m_gpu(config, &m_window)
    , m_renderer(&m_gpu) {
} // TODO: Rework to integrate World/WorldContext/GameInstance

Engine::~Engine() = default; // NOTE: Will later handle waiting for threads and memory freeing

void Engine::tick(Application& application) {
	TRIVIAL_PROFILE_FRAME("Frame");
	m_time.tick();
	const FrameContext kFrameContext = {.deltaTime = m_time.deltaSeconds(), .frameIndex = m_frameIndex};

	internal::platform::Window::pollEvents(); // TODO: Ought to change the form of this

	{
		TRIVIAL_PROFILE_SCOPE("Update game layer");

		application.updateGame(kFrameContext); // NOTE: Later split into stuff like updating physics and other phases
	}

#if TRIVIAL_CONFIG_DEBUG
	{
		TRIVIAL_PROFILE_SCOPE("Update debug layer");
		// Not really needed but will want debug layer to be decent quality later

		application.updateDebug(kFrameContext);
	}
#endif // TRIVIAL_CONFIG_DEBUG

	// have a prepare render function first to keep distinct from update game for clean minimise implementation later
	// rendering

	++m_frameIndex;
}

void Engine::run(Application& application) {
	TRIVIAL_PROFILE_THREAD("Main Thread");

	m_time.reset();

	{
		TRIVIAL_PROFILE_SCOPE("Application Start");
		application.onStart();
	}

	while (!m_window.shouldClose()) {
		tick(application);
	}

	{
		TRIVIAL_PROFILE_SCOPE("Application End");
		application.onEnd();
	}
}

} // namespace trivial

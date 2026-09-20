#include <trivial/engine.h>

#include <cstdlib>

#include <trivial/core/assert.h>
#include <trivial/core/config.h>
#include <trivial/core/log.h>
#include <trivial/core/profile.h>
#include <trivial/core/thread/thread.h>
#include <trivial/task/task.h>

#include "core/sync/parking_lot.h"

namespace trivial {

namespace {

GraphicsApi readRequestedGraphicsApi(const EngineConfig* config) {
	TRIVIAL_ASSERT(config != nullptr);

	return config->graphicsApi;
}

} // namespace

Engine::Engine(const EngineConfig* config) noexcept
    : m_requestedGraphicsApi(readRequestedGraphicsApi(config))
    , m_frameIndex(0)
    , m_window(config)
    , m_gpu(config, &m_window)
    , m_renderer(&m_gpu) {
	auto* mainThread = new (std::nothrow) thread::Thread(); // TODO: Custom allocator
	if (mainThread == nullptr) {
		TRIVIAL_LOG_FATAL_PREFIX("Engine", "Failed to allocate main thread adoption record");
		std::abort();
	}
	mainThread->adoptCurrentThread({.name = "Main", .type = thread::ThreadType::Main});

	task::TaskSystemConfig taskConfig = config->tasks;
	taskConfig.workers.count = thread::Thread::resolveConcurrency(taskConfig.workers.count);

	const std::size_t kCapacity = 1 + taskConfig.workers.count + taskConfig.workers.maxStandbyWorkers;

	auto* parkingLot = new (std::nothrow) sync::ParkingLot(kCapacity); // TODO: Custom allocator
	if (parkingLot == nullptr) {
		TRIVIAL_LOG_FATAL_PREFIX("Engine", "Failed to allocate ParkingLot");
		std::abort();
	}
	sync::setActiveParkingLot(parkingLot);

	auto* taskSystem = new (std::nothrow) task::TaskSystem(taskConfig); // TODO: Custom allocator
	if (taskSystem == nullptr) {
		TRIVIAL_LOG_FATAL_PREFIX("Engine", "Failed to allocate TaskSystem");
		std::abort();
	}
	task::setActiveTaskSystem(taskSystem);
} // TODO: Rework to integrate World/WorldContext/GameInstance

Engine::~Engine() {
	delete &task::activeTaskSystem();
	task::setActiveTaskSystem(nullptr);

	delete &sync::activeParkingLot();
	sync::setActiveParkingLot(nullptr);

	delete thread::Thread::current();
}

void Engine::tick(Application& application) noexcept {
	TRIVIAL_PROFILE_FRAME("Frame");
	m_time.tick();
	const FrameContext kFrameContext = {.deltaTime = m_time.deltaSeconds(), .frameIndex = m_frameIndex};

	platform::Window::pollEvents(); // TODO: Ought to change the form of this

	{
		TRIVIAL_PROFILE_SCOPE("Update game layer");

		application.updateGame(kFrameContext); // NOTE: Later split into stuff like updating physics and other phases
	}

#if TRIVIAL_CONFIG_DEBUG
	{
		TRIVIAL_PROFILE_SCOPE("Update debug layer");

		application.updateDebug(kFrameContext);
	}
#endif // TRIVIAL_CONFIG_DEBUG

	{
		TRIVIAL_PROFILE_SCOPE("Render"); // TODO: Look to do two render passes to profile game and debug separately

		// TODO: Move to world function on ecs
		const std::vector<render::Drawable> kDrawables = application.collectDrawables();
		m_renderer.drawFrame(m_frameIndex, kDrawables);
	}

	++m_frameIndex;
}

void Engine::run(Application& application) noexcept {
	TRIVIAL_PROFILE_THREAD(thread::Thread::current()->name());

	m_time.reset();

	{
		TRIVIAL_PROFILE_SCOPE("Application Start");
		application.onStart(&m_gpu);
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

#include <trivial/engine.h>

#include <cstddef>
#include <vector>

#include <trivial/application.h>
#include <trivial/core/config.h>
#include <trivial/core/log.h>
#include <trivial/core/profile.h>
#include <trivial/core/thread/thread.h>
#include <trivial/frame/frame_context.h>
#include <trivial/platform/window.h>
#include <trivial/render/renderer.h>
#include <trivial/task/task.h>
#include <trivial/task/task_system.h>

#include "core/cpu/cpu_capabilities.h"
#include "core/memory/os/memory_capabilities.h"
#include "core/sync/parking_lot.h"
#include "core/time/time_capabilities.h"

namespace trivial {

Engine::CoreSystems::CoreSystems() noexcept {
	cpu::initCapabilities();
	memory::initCapabilities();
	time::initCapabilities();

	TRIVIAL_LOG_STARTUP_SYSTEM_INFO();
}

Engine::CoreSystems::~CoreSystems() {
	cpu::freeCapabilities();
}

Engine::Engine(const EngineConfig& config) noexcept
    : m_requestedGraphicsApi(config.graphicsApi)
    , m_window(config.window)
    , m_gpu(config.graphicsApi, config.applicationInfo, m_window)
    , m_renderer(&m_gpu) {
	thread::Thread* mainThread = new thread::Thread();
	mainThread->adoptCurrentThread({.name = "Main", .type = thread::ThreadType::Main});

	task::TaskSystemConfig taskConfig = config.tasks;
	taskConfig.workers.count = thread::Thread::resolveConcurrency(taskConfig.workers.count);

	const std::size_t kCapacity = 1 + taskConfig.workers.count + taskConfig.workers.maxStandbyWorkers;

	sync::setActiveParkingLot(new sync::ParkingLot(kCapacity));
	task::setActiveTaskSystem(new task::TaskSystem(taskConfig));
}

Engine::~Engine() {
	delete &task::activeTaskSystem();
	task::setActiveTaskSystem(nullptr);

	delete &sync::activeParkingLot();
	sync::setActiveParkingLot(nullptr);

	delete thread::Thread::current();
}

void Engine::tick(Application& application) noexcept {
	TRIVIAL_PROFILE_FRAME("Frame");
	m_frameTimer.tick();
	const FrameContext kFrameContext = {.deltaTime = m_frameTimer.deltaSeconds(), .frameNumber = m_frameNumber};

	platform::Window::pollEvents();

	{
		TRIVIAL_PROFILE_SCOPE("Update game layer");

		application.updateGame(kFrameContext);
	}

#if TRIVIAL_CONFIG_DEBUG
	{
		TRIVIAL_PROFILE_SCOPE("Update debug layer");

		application.updateDebug(kFrameContext);
	}
#endif // TRIVIAL_CONFIG_DEBUG

	{
		TRIVIAL_PROFILE_SCOPE("Render");

		const std::vector<render::Drawable> kDrawables = application.collectDrawables();
		m_renderer.drawFrame(m_frameNumber, kDrawables);
	}

	++m_frameNumber;
}

void Engine::run(Application& application) noexcept {
	TRIVIAL_PROFILE_THREAD(thread::Thread::current()->name());

	m_frameTimer.reset();

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

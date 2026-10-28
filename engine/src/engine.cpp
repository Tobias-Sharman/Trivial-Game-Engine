#include <trivial/engine.h>

#include <cstddef>
#include <cstdlib>
#include <new>
#include <vector>

#include <trivial/application.h>
#include <trivial/core/config.h>
#include <trivial/core/cpu/cpu.h>
#include <trivial/core/log.h>
#include <trivial/core/platform.h>
#include <trivial/core/profile.h>
#include <trivial/core/thread/thread.h>
#include <trivial/frame/frame_context.h>
#include <trivial/platform/window.h>
#include <trivial/render/renderer.h>
#include <trivial/task/task.h>
#include <trivial/task/task_system.h>

#include "core/cpu/cpu_capabilities.h"
#include "core/memory/memory_capabilities.h"
#include "core/sync/parking_lot.h"
#include "core/time/time_capabilities.h"

namespace trivial {

Engine::Engine(const EngineConfig& config) noexcept
    : m_requestedGraphicsApi(config.graphicsApi)
    , m_window(config.window)
    , m_gpu(config.graphicsApi, config.applicationInfo, m_window)
    , m_renderer(&m_gpu) {
	memory::initCapabilities();
	cpu::initCapabilities();
	time::initCapabilities();

	TRIVIAL_LOG_STARTUP_SYSTEM_INFO();

	thread::Thread* mainThread = new (std::nothrow) thread::Thread();
	if (mainThread == nullptr) {
		TRIVIAL_LOG_FATAL_PREFIX("Engine", "Failed to allocate main thread adoption record");
		std::abort();
	}
	mainThread->adoptCurrentThread({.name = "Main", .type = thread::ThreadType::Main});

	task::TaskSystemConfig taskConfig = config.tasks;
	taskConfig.workers.count = thread::Thread::resolveConcurrency(taskConfig.workers.count);

	const std::size_t kCapacity = 1 + taskConfig.workers.count + taskConfig.workers.maxStandbyWorkers;

	sync::ParkingLot* parkingLot = new (std::nothrow) sync::ParkingLot(kCapacity);
	if (parkingLot == nullptr) {
		TRIVIAL_LOG_FATAL_PREFIX("Engine", "Failed to allocate ParkingLot");
		std::abort();
	}
	sync::setActiveParkingLot(parkingLot);

	task::TaskSystem* taskSystem = new (std::nothrow) task::TaskSystem(taskConfig);
	if (taskSystem == nullptr) {
		TRIVIAL_LOG_FATAL_PREFIX("Engine", "Failed to allocate TaskSystem");
		std::abort();
	}
	task::setActiveTaskSystem(taskSystem);
}

Engine::~Engine() {
	delete &task::activeTaskSystem();
	task::setActiveTaskSystem(nullptr);

	delete &sync::activeParkingLot();
	sync::setActiveParkingLot(nullptr);

	delete thread::Thread::current();

#if TRIVIAL_PLATFORM_HAS_CPU_AFFINITY
	delete[] cpu::detail::g_detectedCapabilities.topology.logicalProcessors;
	delete[] cpu::detail::g_detectedCapabilities.topology.caches;
#else
	delete[] cpu::detail::g_detectedCapabilities.topology.efficiencyClasses;
#endif // TRIVIAL_PLATFORM_HAS_CPU_AFFINITY
	cpu::detail::g_detectedCapabilities.topology = {};
}

void Engine::tick(Application& application) noexcept {
	TRIVIAL_PROFILE_FRAME("Frame");
	m_frameTimer.tick();
	const FrameContext kFrameContext = {.deltaTime = m_frameTimer.deltaSeconds(), .frameIndex = m_frameIndex};

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
		m_renderer.drawFrame(m_frameIndex, kDrawables);
	}

	++m_frameIndex;
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

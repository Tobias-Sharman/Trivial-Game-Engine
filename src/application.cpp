#include <trivial/application.h>

#include <utility>

#include <trivial/core/assert.h>
#include <trivial/core/config.h>

namespace trivial {

Application::Application(std::unique_ptr<Layer> gameLayer) noexcept
    : m_gameLayer(std::move(gameLayer)) {

	TRIVIAL_ASSERT(m_gameLayer != nullptr);
}

#if TRIVIAL_CONFIG_DEBUG
void Application::attachDebugLayer(std::unique_ptr<Layer> debugLayer) noexcept {
	m_debugLayer = std::move(debugLayer);
}
#endif // TRIVIAL_CONFIG_DEBUG

void Application::onStart(gpu::Context* gpu) noexcept {
	TRIVIAL_ASSERT(m_gameLayer != nullptr);
	m_gameLayer->onStart(gpu);

#if TRIVIAL_CONFIG_DEBUG
	TRIVIAL_ASSERT(m_debugLayer != nullptr);
	m_debugLayer->onStart(gpu);
#endif // TRIVIAL_CONFIG_DEBUG
}

void Application::onEnd() noexcept {
	TRIVIAL_ASSERT(m_gameLayer != nullptr);
	m_gameLayer->onEnd();

#if TRIVIAL_CONFIG_DEBUG
	TRIVIAL_ASSERT(m_debugLayer != nullptr);
	m_debugLayer->onEnd();
#endif // TRIVIAL_CONFIG_DEBUG
}

void Application::updateGame(const FrameContext& frameContext) noexcept {
	TRIVIAL_ASSERT(m_gameLayer != nullptr);
	m_gameLayer->onUpdate(frameContext);
}

#if TRIVIAL_CONFIG_DEBUG
void Application::updateDebug(const FrameContext& frameContext) noexcept {
	TRIVIAL_ASSERT(m_debugLayer != nullptr);
	m_debugLayer->onUpdate(frameContext);
}
#endif // TRIVIAL_CONFIG_DEBUG

[[nodiscard]] std::vector<render::Drawable> Application::collectDrawables() const noexcept {
	TRIVIAL_ASSERT(m_gameLayer != nullptr);
	std::vector<render::Drawable> drawables = m_gameLayer->collectDrawables();

#if TRIVIAL_CONFIG_DEBUG
	TRIVIAL_ASSERT(m_debugLayer != nullptr);
	std::vector<render::Drawable> debugDrawables2 = m_debugLayer->collectDrawables();
	drawables.insert(drawables.end(), debugDrawables2.begin(), debugDrawables2.end());
#endif // TRIVIAL_CONFIG_DEBUG

	return drawables;
}

} // namespace trivial

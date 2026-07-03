#include <trivial/application.h>

#include <utility>

#include <trivial/core/config.h>

#include "core/assert.h"

namespace trivial {

Application::Application(std::unique_ptr<Layer> gameLayer)
    : m_gameLayer(std::move(gameLayer)) {

	TRIVIAL_ASSERT(m_gameLayer != nullptr);
}

#if TRIVIAL_CONFIG_DEBUG
void Application::attachDebugLayer(std::unique_ptr<Layer> debugLayer) {
	m_debugLayer = std::move(debugLayer);
}
#endif // TRIVIAL_CONFIG_DEBUG

void Application::onStart() {
	TRIVIAL_ASSERT(m_gameLayer != nullptr);
	m_gameLayer->onStart();

#if TRIVIAL_CONFIG_DEBUG
	TRIVIAL_ASSERT(m_debugLayer != nullptr);
	m_debugLayer->onStart();
#endif // TRIVIAL_CONFIG_DEBUG
}

void Application::updateGame(const FrameContext& frameContext) {
	TRIVIAL_ASSERT(m_gameLayer != nullptr);
	m_gameLayer->onUpdate(frameContext);
}

#if TRIVIAL_CONFIG_DEBUG
void Application::updateDebug(const FrameContext& frameContext) {
	TRIVIAL_ASSERT(m_debugLayer != nullptr);
	m_debugLayer->onUpdate(frameContext);
}
#endif // TRIVIAL_CONFIG_DEBUG

void Application::onEnd() {
	TRIVIAL_ASSERT(m_gameLayer != nullptr);
	m_gameLayer->onEnd();

#if TRIVIAL_CONFIG_DEBUG
	TRIVIAL_ASSERT(m_debugLayer != nullptr);
	m_debugLayer->onEnd();
#endif // TRIVIAL_CONFIG_DEBUG
}

} // namespace trivial

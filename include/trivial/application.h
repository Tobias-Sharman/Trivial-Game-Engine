#ifndef TRIVIAL_APPLICATION_H
#define TRIVIAL_APPLICATION_H

#include <memory>

#include <trivial/layers.h>
#include <trivial/core/config.h>
#include <trivial/frame/frame_context.h>

namespace trivial {
// NOTE: Need to decide if wanting to support more layers, could even be a vector of them used like a stack
class Application {
public:
	Application() = delete;
	explicit Application(std::unique_ptr<Layer> gameLayer);

	virtual ~Application() = default;

	Application(const Application&) = delete;
	Application& operator=(const Application) = delete;

	Application(Application&&) = delete;
	Application& operator=(Application&&) = delete;

#if TRIVIAL_CONFIG_DEBUG
	void attachDebugLayer(std::unique_ptr<Layer> debugLayer);
#endif // TRIVIAL_CONFIG_DEBUG

	void onStart();
	void onEnd();

	void updateGame(const FrameContext& frameContext);
#if TRIVIAL_CONFIG_DEBUG
	void updateDebug(const FrameContext& frameContext);
#endif // TRIVIAL_CONFIG_DEBUG

private:
	std::unique_ptr<Layer> m_gameLayer;

#if TRIVIAL_CONFIG_DEBUG
	std::unique_ptr<Layer> m_debugLayer;
#endif // TRIVIAL_CONFIG_DEBUG
};

} // namespace trivial

#if TRIVIAL_CONFIG_DEBUG
#define TRIVIAL_ATTACH_DEBUG_LAYER(app, expr) (app).attachDebugLayer(expr)
#else
#define TRIVIAL_ATTACH_DEBUG_LAYER(app, exp) ((void)0)
#endif // TRIVIAL_CONFIG_DEBUG

#endif // TRIVIAL_APPLICATION_H

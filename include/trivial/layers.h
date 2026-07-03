#ifndef TRIVIAL_LAYERS_H
#define TRIVIAL_LAYERS_H

#include <trivial/frame/frame_context.h>

namespace trivial {

class Layer {
public:
	Layer() = default;

	virtual ~Layer() = default;

	Layer(const Layer&) = delete;
	Layer& operator=(const Layer&) = delete;

	Layer(Layer&&) = delete;
	Layer& operator=(Layer&&) = delete;

	virtual void onStart() {};
	virtual void onUpdate(const FrameContext& frameContext) {};
	virtual void onEnd() {};
};

} // namespace trivial

#endif // TRIVIAL_LAYERS_H

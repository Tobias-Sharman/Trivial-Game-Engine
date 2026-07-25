#ifndef TRIVIAL_LAYERS_H
#define TRIVIAL_LAYERS_H

#include <vector>

#include <trivial/frame/frame_context.h>
#include <trivial/gpu/context.h>
#include <trivial/render/renderer.h>

namespace trivial {

class Layer {
public:
	Layer() = default;

	virtual ~Layer() = default;

	Layer(const Layer&) = delete;
	Layer& operator=(const Layer&) = delete;

	Layer(Layer&&) = delete;
	Layer& operator=(Layer&&) = delete;

	virtual void onStart(gpu::Context* gpu) noexcept {};
	virtual void onUpdate(const FrameContext& frameContext) noexcept {};
	virtual void onEnd() noexcept {};

	[[nodiscard]] virtual std::vector<render::Drawable> collectDrawables() const noexcept = 0;
};

} // namespace trivial

#endif // TRIVIAL_LAYERS_H

#ifndef TRIVIAL_RENDER_RENDERER_H
#define TRIVIAL_RENDER_RENDERER_H

#include <trivial/gpu/context.h>

namespace trivial::render {

class Renderer {
public:
	Renderer() = delete;
	explicit Renderer(gpu::Context* gpu);

	~Renderer();

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;

	Renderer(Renderer&&) = delete;
	Renderer& operator=(Renderer&&) = delete;

private:
	gpu::Context* m_gpu = nullptr;
};

} // namespace trivial::render

#endif // TRIVIAL_RENDER_RENDERER_H

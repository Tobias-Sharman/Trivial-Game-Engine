#ifndef TRIVIAL_RENDER_RENDERER_H
#define TRIVIAL_RENDER_RENDERER_H

#include <trivial/gpu/context.h>

namespace trivial::render {

struct Drawable {
	rhi::MeshHandle mesh = 0;
	math::Affine2f transform = math::Affine2f::identity();
	math::Vec4f tint{.x = 1.0F, .y = 1.0F, .z = 1.0F, .w = 1.0F};
};

class Renderer {
public:
	explicit Renderer(gpu::Context* gpu);

	~Renderer() = default;

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;

	Renderer(Renderer&&) = delete;
	Renderer& operator=(Renderer&&) = delete;

	void drawFrame(std::uint64_t frameIndex, std::span<const Drawable> drawables) noexcept;

private:
	gpu::Context* m_gpu = nullptr;
};

} // namespace trivial::render

#endif // TRIVIAL_RENDER_RENDERER_H

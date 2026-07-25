#include <trivial/render/renderer.h>

#include <trivial/core/assert.h>

namespace trivial::render {

Renderer::Renderer(gpu::Context* gpu)
    : m_gpu(gpu) {
	TRIVIAL_ASSERT(m_gpu != nullptr);
}

void Renderer::drawFrame(std::uint64_t frameIndex, std::span<const Drawable> drawables) noexcept {
	if (!m_gpu->beginFrame(frameIndex)) {
		return;
	}

	for (const Drawable& drawable : drawables) {
		m_gpu->drawMesh(drawable.mesh, drawable.transform, drawable.tint);
	}

	m_gpu->endFrame();
}

} // namespace trivial::render

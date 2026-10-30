#include <trivial/render/renderer.h>

#include <cstdint>
#include <span>

#include <trivial/core/assert.h>
#include <trivial/gpu/context.h>

namespace trivial::render {

Renderer::Renderer(gpu::Context* gpu) noexcept
    : m_gpu(gpu) {
	TRIVIAL_ASSERT(m_gpu != nullptr);
}

void Renderer::drawFrame(std::uint64_t frameNumber, std::span<const Drawable> drawables) noexcept {
	if (!m_gpu->beginFrame(frameNumber)) {
		return;
	}

	for (const Drawable& drawable : drawables) {
		m_gpu->drawMesh(drawable.mesh, drawable.transform, drawable.tint);
	}

	m_gpu->endFrame();
}

} // namespace trivial::render

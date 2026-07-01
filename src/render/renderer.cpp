#include <trivial/render/renderer.h>

#include "core/assert.h"

namespace trivial::render {

Renderer::Renderer(internal::gpu::Context* gpu)
    : m_gpu(gpu) {
	TRIVIAL_ASSERT(m_gpu != nullptr);
}

Renderer::~Renderer() = default;

} // namespace trivial::render

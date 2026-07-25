#ifndef TRIVIAL_GPU_CONTEXT_H
#define TRIVIAL_GPU_CONTEXT_H

#include <memory>

#include <trivial/engine_config.h>
#include <trivial/platform/window.h>
#include <trivial/rhi/backend.h>
#include <trivial/rhi/mesh_types.h>

namespace trivial::gpu {

class Context {
public:
	explicit Context(const EngineConfig* config, platform::Window* window);

	~Context();

	Context(const Context&) = delete;
	Context& operator=(const Context&) = delete;

	Context(Context&&) = delete;
	Context& operator=(Context&&) = delete;

	[[nodiscard]] rhi::MeshHandle createMesh(std::span<const rhi::Vertex2> vertices,
	                                         std::span<const std::uint16_t> indices) noexcept {
		return m_backend->createMesh(vertices, indices);
	}

	[[nodiscard]] bool beginFrame(std::uint64_t frameIndex) noexcept { return m_backend->beginFrame(frameIndex); }
	void drawMesh(rhi::MeshHandle handle, const math::Affine2f& transform, const math::Vec4f& tint) noexcept {
		m_backend->drawMesh(handle, transform, tint);
	}
	void endFrame() noexcept { m_backend->endFrame(); }

	[[nodiscard]] GraphicsApi activeGraphicsApi() const { return m_backend->graphicsApi(); }

	void waitIdle();

private:
	std::unique_ptr<rhi::Backend> m_backend;
};

} // namespace trivial::gpu

#endif // TRIVIAL_GPU_CONTEXT_H

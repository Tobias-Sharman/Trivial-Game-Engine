#ifndef TRIVIAL_SRC_RHI_BACKEND_H
#define TRIVIAL_SRC_RHI_BACKEND_H

#include <cstdint>
#include <span>

#include <trivial/engine_config.h>
#include <trivial/core/graphics_api.h>
#include <trivial/core/math/affine2.h>
#include <trivial/core/math/vec4.h>
#include <trivial/rhi/mesh_types.h>

namespace trivial::rhi {

class Backend {
public:
	Backend() noexcept = default;

	virtual ~Backend() = default;

	Backend(const Backend&) = delete;
	Backend& operator=(const Backend&) = delete;

	Backend(Backend&&) = delete;
	Backend& operator=(Backend&&) = delete;

	[[nodiscard]] virtual bool beginFrame(std::uint64_t frameIndex) noexcept = 0;
	virtual void endFrame() noexcept = 0;

	virtual MeshHandle createMesh(std::span<const Vertex2> vertices, std::span<const std::uint16_t> indices) noexcept
	    = 0;
	virtual void drawMesh(MeshHandle handle, const math::Affine2f& transform, const math::Vec4f& tint) noexcept = 0;
	virtual void destroyMesh(MeshHandle handle) noexcept = 0;

	[[nodiscard]] virtual GraphicsApi graphicsApi() const noexcept = 0;

	virtual void waitIdle() noexcept = 0;

	virtual void resize(WindowSize size) noexcept = 0;
};

} // namespace trivial::rhi

#endif // TRIVIAL_SRC_RHI_BACKEND_H

#include <trivial/internal/gpu/context.h>

#include <trivial/core/assert.h>
#include <trivial/core/log.h>
#include <trivial/internal/rhi/vulkan/backend.h>

namespace trivial::internal::gpu {

namespace {

GraphicsApi resolveGraphicsApi(const EngineConfig* config) {
	TRIVIAL_ASSERT(config != nullptr);

	// TODO: Select based on system once there is support for multiple apis
	if (config->graphicsApi == GraphicsApi::Auto) {
		return GraphicsApi::Vulkan;
	}

	return config->graphicsApi;
}

std::unique_ptr<rhi::Backend> createBackend(const EngineConfig* config, platform::Window* window) {
	TRIVIAL_ASSERT(config != nullptr);
	TRIVIAL_ASSERT(window != nullptr);

	const GraphicsApi kGraphicsApi = resolveGraphicsApi(config);

	switch (kGraphicsApi) {
		case GraphicsApi::Vulkan:
			return std::make_unique<rhi::vulkan::Backend>(config, window);

		case GraphicsApi::Auto:
			TRIVIAL_LOG_ERROR("Auto graphics api selection did not pick an api");
			TRIVIAL_ASSERT(kGraphicsApi != GraphicsApi::Auto);
			return nullptr;
	}
}

} // namespace

Context::Context(const EngineConfig* config, platform::Window* window)
    : m_backend(createBackend(config, window)) {
	TRIVIAL_ASSERT(m_backend != nullptr);
}

Context::~Context() = default;

void Context::waitIdle() {
	TRIVIAL_ASSERT(m_backend != nullptr);

	m_backend->waitIdle();
}

} // namespace trivial::internal::gpu

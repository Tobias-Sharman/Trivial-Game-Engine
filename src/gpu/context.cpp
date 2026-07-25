#include <trivial/gpu/context.h>

#include <trivial/core/assert.h>
#include <trivial/core/log.h>

#include "rhi/vulkan/backend.h"

namespace {

trivial::GraphicsApi resolveGraphicsApi(const trivial::EngineConfig* config) {
	TRIVIAL_ASSERT(config != nullptr);

	// TODO: Select based on system once there is support for multiple apis
	if (config->graphicsApi == trivial::GraphicsApi::Auto) {
		return trivial::GraphicsApi::Vulkan;
	}

	return config->graphicsApi;
}

std::unique_ptr<trivial::rhi::Backend> createBackend(const trivial::EngineConfig* config,
                                                     trivial::platform::Window* window) {
	TRIVIAL_ASSERT(config != nullptr);
	TRIVIAL_ASSERT(window != nullptr);

	const trivial::GraphicsApi kGraphicsApi = resolveGraphicsApi(config);

	switch (kGraphicsApi) {
		case trivial::GraphicsApi::Vulkan:
			return std::make_unique<trivial::rhi::vulkan::Backend>(config, window);

		case trivial::GraphicsApi::Auto:
			TRIVIAL_LOG_ERROR("Auto graphics api selection did not pick an api");
			TRIVIAL_ASSERT(kGraphicsApi != trivial::GraphicsApi::Auto);
			return nullptr;
	}
}

} // namespace

namespace trivial::gpu {

Context::Context(const EngineConfig* config, platform::Window* window)
    : m_backend(createBackend(config, window)) {
	TRIVIAL_ASSERT(m_backend != nullptr);
}

Context::~Context() = default;

void Context::waitIdle() {
	TRIVIAL_ASSERT(m_backend != nullptr);

	m_backend->waitIdle();
}

} // namespace trivial::gpu

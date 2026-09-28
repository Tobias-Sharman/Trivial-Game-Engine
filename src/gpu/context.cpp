#include <trivial/gpu/context.h>

#include <trivial/core/assert.h>
#include <trivial/core/log.h>

#include "rhi/vulkan/backend.h"

namespace {

trivial::GraphicsApi resolveGraphicsApi(trivial::GraphicsApi requested) {
	// TODO: Select based on system once there is support for multiple apis
	if (requested == trivial::GraphicsApi::Auto) {
		return trivial::GraphicsApi::Vulkan;
	}

	return requested;
}

std::unique_ptr<trivial::rhi::Backend> createBackend(trivial::GraphicsApi graphicsApi,
                                                     const trivial::ApplicationInfo& applicationInfo,
                                                     const trivial::platform::Window& window) {
	const trivial::GraphicsApi kGraphicsApi = resolveGraphicsApi(graphicsApi);

	switch (kGraphicsApi) {
		case trivial::GraphicsApi::Vulkan:
			return std::make_unique<trivial::rhi::vulkan::Backend>(applicationInfo, window);

		case trivial::GraphicsApi::Auto:
			TRIVIAL_LOG_ERROR("Auto graphics api selection did not pick an api");
			TRIVIAL_ASSERT(kGraphicsApi != trivial::GraphicsApi::Auto);
			return nullptr;
	}
}

} // namespace

namespace trivial::gpu {

Context::Context(GraphicsApi graphicsApi, const ApplicationInfo& applicationInfo, const platform::Window& window)
    : m_backend(createBackend(graphicsApi, applicationInfo, window)) {
	TRIVIAL_ASSERT(m_backend != nullptr);
}

Context::~Context() = default;

void Context::waitIdle() {
	TRIVIAL_ASSERT(m_backend != nullptr);

	m_backend->waitIdle();
}

} // namespace trivial::gpu

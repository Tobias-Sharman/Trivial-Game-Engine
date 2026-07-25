#ifndef TRIVIAL_ENGINE_CONFIG_H
#define TRIVIAL_ENGINE_CONFIG_H

#include <cstdint>
#include <string>

#include <trivial/core/graphics_api.h>
#include <trivial/task/task_system_config.h>

namespace trivial {

struct Version {
	std::uint32_t major = 0;
	std::uint32_t minor = 1;
	std::uint32_t patch = 0;
};

struct WindowSize {
	std::uint32_t height = 720;
	std::uint32_t width = 1280;
};

struct WindowConfig {
	WindowSize size = {};
	std::string title = "Trivial";
};

struct EngineConfig {
	GraphicsApi graphicsApi = GraphicsApi::Auto;

	std::string applicationName = "Trivial Application";
	Version applicationVersion = {};

	std::string engineName = "Trivial";
	Version engineVersion = {}; // TODO: Later don't expose this as user config

	WindowConfig window = {};

	task::TaskSystemConfig tasks = {};
};

} // namespace trivial

#endif // TRIVIAL_ENGINE_CONFIG_H

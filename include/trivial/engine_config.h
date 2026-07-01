#ifndef TRIVIAL_ENGINE_CONFIG_H
#define TRIVIAL_ENGINE_CONFIG_H

#include <string>

#include <trivial/core/graphics_api.h>

namespace trivial {

struct Version {
	int major = 0;
	int minor = 1;
	int patch = 0;
};

// TODO: add general config file that would allow for easily switching between standard resolution sizes
//           this is to come back to once the window system is in place in case of api changes
//           maybe add to the build command options
//       Do with J
struct WindowSize {
	int height = 720;
	int width = 1280;
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
	Version engineVersion
	    = {}; // TODO: Later probably have this set not by the user of the engine as I going static with it

	WindowConfig window = {};
};

} // namespace trivial

#endif // TRIVIAL_ENGINE_CONFIG_H

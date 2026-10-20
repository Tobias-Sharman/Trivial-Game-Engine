#ifndef TRIVIAL_CORE_APPLICATION_INFO_H
#define TRIVIAL_CORE_APPLICATION_INFO_H

#include <cstdint>
#include <string>

namespace trivial {

struct Version {
	std::uint32_t major = 0;
	std::uint32_t minor = 0;
	std::uint32_t patch = 0;
};

struct ApplicationInfo {
	std::string name;
	Version version = {};
};

} // namespace trivial

#endif // TRIVIAL_CORE_APPLICATION_INFO_H

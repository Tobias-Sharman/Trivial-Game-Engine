#ifndef TRIVIAL_CORE_MEMORY_OOM_HANDLER_H
#define TRIVIAL_CORE_MEMORY_OOM_HANDLER_H

#include <cstddef>

namespace trivial::memory {

struct OomInfo {
	std::size_t requestedSize;
	const char* context;
	int osErrorCode;
};

using OomHandler = void (*)(const OomInfo& info);

} // namespace trivial::memory

#endif // TRIVIAL_CORE_MEMORY_OOM_HANDLER_H

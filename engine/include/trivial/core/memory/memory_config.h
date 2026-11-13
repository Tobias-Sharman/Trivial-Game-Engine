#ifndef TRIVIAL_CORE_MEMORY_MEMORY_CONFIG_H
#define TRIVIAL_CORE_MEMORY_MEMORY_CONFIG_H

#include <trivial/core/config.h> // IWYU pragma: export

#ifndef TRIVIAL_MEMORY_DEBUG_LEVEL
#if TRIVIAL_CONFIG_DEBUG
#define TRIVIAL_MEMORY_DEBUG_LEVEL 2
#elif TRIVIAL_CONFIG_RELWITHDEBINFO
#define TRIVIAL_MEMORY_DEBUG_LEVEL 1
#else
#define TRIVIAL_MEMORY_DEBUG_LEVEL 0
#endif
#endif

// NOTE: For use cases where returning memory has no benefit to the system,
//       here this being expected stuff for consoles or maybe stuff like a
//       deployment for a headless compute only operation running on a system
//       where your whole process gets a dedicated pool of physical memory
#ifndef TRIVIAL_MEMORY_ENABLE_DECOMMIT
#define TRIVIAL_MEMORY_ENABLE_DECOMMIT 1
#endif

#ifndef TRIVIAL_MEMORY_PREFER_LAZY_DECOMMIT
#define TRIVIAL_MEMORY_PREFER_LAZY_DECOMMIT 0
#endif

#ifndef TRIVIAL_MEMORY_DEFAULT_PHYSICAL_MEMORY_BYTES
#define TRIVIAL_MEMORY_DEFAULT_PHYSICAL_MEMORY_BYTES 34'359'738'368 // 32 GiB
#endif

#if (TRIVIAL_MEMORY_DEBUG_LEVEL < 0) || (TRIVIAL_MEMORY_DEBUG_LEVEL > 3)
#error "TRIVIAL_MEMORY_DEBUG_LEVEL must be 0 to 3"
#endif

#if (TRIVIAL_MEMORY_ENABLE_DECOMMIT != 0) && (TRIVIAL_MEMORY_ENABLE_DECOMMIT != 1)
#error "TRIVIAL_MEMORY_ENABLE_DECOMMIT must be 0 or 1"
#endif

#if (TRIVIAL_MEMORY_PREFER_LAZY_DECOMMIT != 0) && (TRIVIAL_MEMORY_PREFER_LAZY_DECOMMIT != 1)
#error "TRIVIAL_MEMORY_PREFER_LAZY_DECOMMIT must be 0 or 1"
#endif

#if TRIVIAL_MEMORY_DEFAULT_PHYSICAL_MEMORY_BYTES < 1
#error "TRIVIAL_MEMORY_DEFAULT_PHYSICAL_MEMORY_BYTES must be at least 1"
#endif

#endif // TRIVIAL_CORE_MEMORY_MEMORY_CONFIG_H

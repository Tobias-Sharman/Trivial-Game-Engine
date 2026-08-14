#ifndef TRIVIAL_CORE_PROFILE_H
#define TRIVIAL_CORE_PROFILE_H

#include <trivial/core/config.h>

#if TRIVIAL_ENABLE_TRACY

#include <tracy/Tracy.hpp>

#define TRIVIAL_PROFILE_SCOPE(name) ZoneScopedN(name)
#define TRIVIAL_PROFILE_FUNCTION() ZoneScoped
#define TRIVIAL_PROFILE_FRAME(name) FrameMarkNamed(name)
#define TRIVIAL_PROFILE_THREAD(name) tracy::SetThreadName(name)
#define TRIVIAL_PROFILE_VALUE(name, value) TracyPlot(name, value)

#define TRIVIAL_PROFILE_ALLOC(pool, ptr, size) TracyAllocN(ptr, size, pool)
#define TRIVIAL_PROFILE_FREE(pool, ptr) TracyFreeN(ptr, pool)

#else

#define TRIVIAL_PROFILE_SCOPE(name) ((void)0)
#define TRIVIAL_PROFILE_FUNCTION() ((void)0)
#define TRIVIAL_PROFILE_FRAME(name) ((void)0)
#define TRIVIAL_PROFILE_THREAD(name) ((void)0)
#define TRIVIAL_PROFILE_VALUE(name, value) ((void)0)

#define TRIVIAL_PROFILE_ALLOC(pool, ptr, size) ((void)0)
#define TRIVIAL_PROFILE_FREE(pool, ptr) ((void)0)

#endif // TRIVIAL_ENABLE_TRACY

#endif // TRIVIAL_CORE_PROFILE_H

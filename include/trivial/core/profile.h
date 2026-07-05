#ifndef TRIVIAL_SRC_CORE_PROFILE_H
#define TRIVIAL_SRC_CORE_PROFILE_H

#include <trivial/core/config.h>

#if TRIVIAL_ENABLE_TRACY

#include <tracy/Tracy.hpp>

#define TRIVIAL_PROFILE_SCOPE(name) ZoneScopedN(name)
#define TRIVIAL_PROFILE_FUNCTION() ZoneScoped
#define TRIVIAL_PROFILE_FRAME(name) FrameMarkNamed(name)
#define TRIVIAL_PROFILE_THREAD(name) tracy::SetThreadName(name)
#define TRIVIAL_PROFILE_VALUE(name, value) TracyPlot(name, value)

#else

#define TRIVIAL_PROFILE_SCOPE(name) ((void)0)
#define TRIVIAL_PROFILE_FUNCTION() ((void)0)
#define TRIVIAL_PROFILE_FRAME(name) ((void)0)
#define TRIVIAL_PROFILE_THREAD(name) ((void)0)
#define TRIVIAL_PROFILE_VALUE(name, value) ((void)0)

#endif // TRIVIAL_ENABLE_TRACY

#endif // TRIVIAL_SRC_CORE_PROFILE_H

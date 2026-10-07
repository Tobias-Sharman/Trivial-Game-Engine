#ifndef TRIVIAL_FRAME_FRAME_CONFIG_H
#define TRIVIAL_FRAME_FRAME_CONFIG_H

#include <trivial/core/config.h> // IWYU pragma: keep

#ifndef TRIVIAL_FRAME_MAX_DELTA_MILLISECONDS
#define TRIVIAL_FRAME_MAX_DELTA_MILLISECONDS 250
#endif

static_assert(TRIVIAL_FRAME_MAX_DELTA_MILLISECONDS > 0, "Maximum delta must be positive");

#endif // TRIVIAL_FRAME_FRAME_CONFIG_H

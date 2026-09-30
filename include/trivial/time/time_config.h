#ifndef TRIVIAL_TIME_TIME_CONFIG_H
#define TRIVIAL_TIME_TIME_CONFIG_H

#include <trivial/core/config.h> // IWYU pragma: keep

#ifndef TRIVIAL_TIME_MAX_DELTA_SECONDS
#define TRIVIAL_TIME_MAX_DELTA_SECONDS 0.25
#endif

static_assert(TRIVIAL_TIME_MAX_DELTA_SECONDS > 0.0, "Maximum delta must be positive");

#endif // TRIVIAL_TIME_TIME_CONFIG_H

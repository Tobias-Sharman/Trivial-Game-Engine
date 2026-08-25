#ifndef TRIVIAL_CORE_SYNC_SYNC_CONFIG_H
#define TRIVIAL_CORE_SYNC_SYNC_CONFIG_H

#include <trivial/core/config.h>

// Defaults from spinwait.rs in the parking_lot_core crate - checked 20-08-2026
#ifndef TRIVIAL_SYNC_SPIN_COUNT_BEFORE_YIELD
#define TRIVIAL_SYNC_SPIN_COUNT_BEFORE_YIELD 3
#endif

#ifndef TRIVIAL_SYNC_MAX_SPIN_COUNT
#define TRIVIAL_SYNC_MAX_SPIN_COUNT 10
#endif

#ifndef TRIVIAL_SYNC_MIN_PAUSE_ITERATIONS
#define TRIVIAL_SYNC_MIN_PAUSE_ITERATIONS 2
#endif

static_assert(TRIVIAL_SYNC_SPIN_COUNT_BEFORE_YIELD < TRIVIAL_SYNC_MAX_SPIN_COUNT,
              "Must escalate to yielding before hitting the spin limit");

#endif // TRIVIAL_CORE_SYNC_SYNC_CONFIG_H

#ifndef TRIVIAL_CORE_SYNC_SYNC_CONFIG_H
#define TRIVIAL_CORE_SYNC_SYNC_CONFIG_H

#include <trivial/core/config.h> // IWYU pragma: keep

// Defaults from spinwait.rs in the parking_lot_core crate - checked 20-08-2026
#ifndef TRIVIAL_SYNC_SPIN_COUNT_BEFORE_YIELD
#define TRIVIAL_SYNC_SPIN_COUNT_BEFORE_YIELD 3U
#endif

#ifndef TRIVIAL_SYNC_MAX_SPIN_COUNT
#define TRIVIAL_SYNC_MAX_SPIN_COUNT 10U
#endif

#ifndef TRIVIAL_SYNC_MIN_PAUSE_ITERATIONS
#define TRIVIAL_SYNC_MIN_PAUSE_ITERATIONS 2U
#endif

static_assert(TRIVIAL_SYNC_SPIN_COUNT_BEFORE_YIELD < TRIVIAL_SYNC_MAX_SPIN_COUNT,
              "Must escalate to yielding before hitting the spin limit");

// Default from parking_lot.rs in the parking_lot_core crate - checked 10-09-2026
#ifndef TRIVIAL_SYNC_PARKING_LOT_LOAD_FACTOR
#define TRIVIAL_SYNC_PARKING_LOT_LOAD_FACTOR 3U
#endif

#endif // TRIVIAL_CORE_SYNC_SYNC_CONFIG_H

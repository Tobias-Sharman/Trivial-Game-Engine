#ifndef TRIVIAL_SRC_CORE_SYNC_PARKING_LOT_SLOT_H
#define TRIVIAL_SRC_CORE_SYNC_PARKING_LOT_SLOT_H

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>

#include <trivial/core/platform.h>

#include "core/sync/parker.h"

#define TRIVIAL_SYNC_PARKING_LOT_INVALID_SLOT_INDEX (std::numeric_limits<std::size_t>::max())

namespace trivial::sync {

struct alignas(TRIVIAL_PLATFORM_FALSE_SHARING_ALIGNMENT) ParkingLotSlot {
	std::atomic<std::uintptr_t> key{0};
	Parker parker;

	std::size_t nextInQueue = TRIVIAL_SYNC_PARKING_LOT_INVALID_SLOT_INDEX;
};

} // namespace trivial::sync

#endif // TRIVIAL_SRC_CORE_SYNC_PARKING_LOT_SLOT_H

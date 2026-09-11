#ifndef TRIVIAL_SRC_CORE_SYNC_BUCKET_H
#define TRIVIAL_SRC_CORE_SYNC_BUCKET_H

#include <cstddef>

#include <trivial/core/platform.h>

#include "core/sync/escalating_lock.h"
#include "core/sync/parking_lot_slot.h"

namespace trivial::sync {

struct alignas(TRIVIAL_PLATFORM_FALSE_SHARING_ALIGNMENT) Bucket {
	EscalatingLock lock;

	std::size_t queueHead = g_kInvalidParkingLotSlotIndex;
	std::size_t queueTail = g_kInvalidParkingLotSlotIndex;
};

} // namespace trivial::sync

#endif // TRIVIAL_SRC_CORE_SYNC_BUCKET_H

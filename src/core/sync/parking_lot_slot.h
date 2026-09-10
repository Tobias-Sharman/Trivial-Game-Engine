#ifndef TRIVIAL_SRC_CORE_SYNC_PARKING_LOT_SLOT_H
#define TRIVIAL_SRC_CORE_SYNC_PARKING_LOT_SLOT_H

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>

#include <trivial/core/assert.h>
#include <trivial/core/platform.h>

#include "core/sync/parker.h"

namespace trivial::sync {

struct alignas(TRIVIAL_PLATFORM_FALSE_SHARING_ALIGNMENT) ParkingLotSlot {
	std::atomic<std::uintptr_t> key{0};
	Parker parker;
};

class ParkingLotSlotArray {
public:
	explicit ParkingLotSlotArray(std::size_t size) noexcept
	    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
	    : m_slots(std::make_unique<ParkingLotSlot[]>(size))
	    , m_size(size) {}

	~ParkingLotSlotArray() noexcept = default;

	ParkingLotSlotArray(const ParkingLotSlotArray&) = delete;
	ParkingLotSlotArray& operator=(const ParkingLotSlotArray&) = delete;

	ParkingLotSlotArray(ParkingLotSlotArray&&) = delete;
	ParkingLotSlotArray& operator=(ParkingLotSlotArray&&) = delete;

	[[nodiscard]] ParkingLotSlot& operator[](std::size_t index) noexcept {
		TRIVIAL_ASSERT(index < m_size);
		return m_slots[index]; // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
	}

	[[nodiscard]] const ParkingLotSlot& operator[](std::size_t index) const noexcept {
		TRIVIAL_ASSERT(index < m_size);
		return m_slots[index]; // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
	}

	[[nodiscard]] std::size_t size() const noexcept { return m_size; }

private:
	// TODO: Custom allocator
	std::unique_ptr<ParkingLotSlot[]> m_slots; // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
	std::size_t m_size;
};

} // namespace trivial::sync

#endif // TRIVIAL_SRC_CORE_SYNC_PARKING_LOT_SLOT_H

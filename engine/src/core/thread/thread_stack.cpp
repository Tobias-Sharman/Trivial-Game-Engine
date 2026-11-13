#include <trivial/core/thread/thread_stack.h>

#include <trivial/core/platform.h>

#if TRIVIAL_PLATFORM_POSIX

#include <cstddef>

#include <trivial/core/assert.h>
#include <trivial/core/memory/alignment.h>
#include <trivial/core/memory/byte_count.h>
#include <trivial/core/memory/memory.h>
#include <trivial/core/memory/pointer_arithmetic.h>

#include "core/memory/os/os_memory.h"

namespace trivial::thread {

[[nodiscard]] bool allocateStack(memory::ByteCount stackSize,
                                 ThreadStackAllocation& outAllocation,
                                 int& outOsErrorCode) noexcept {
	TRIVIAL_ASSERT(!stackSize.isZero());

	const memory::ByteCount kOsPageSize = memory::osPageSize();
	const memory::ByteCount kAlignment = memory::osAllocationGranularity();

	const memory::ByteCount kGuardSize = kOsPageSize;
	const memory::ByteCount kTotalSize
	    = memory::alignUp(kGuardSize + memory::alignUp(stackSize, kOsPageSize), kAlignment);
	const memory::ByteCount kStackSize = kTotalSize - kGuardSize;

	void* const kBase = memory::reserveAligned(kTotalSize, kAlignment, outOsErrorCode);

	if (kBase == nullptr) {
		return false;
	}

	if (!memory::makePagesNoAccess(kBase, kGuardSize, outOsErrorCode)) {
		memory::releaseReservation(kBase, kTotalSize);

		return false;
	}

	void* const kStackBase = memory::pointerPlus(kBase, kGuardSize);

	if (!memory::commitPages(kStackBase, kStackSize, outOsErrorCode)) {
		memory::releaseReservation(kBase, kTotalSize);

		return false;
	}

	outAllocation.stackBase = kStackBase;
	outAllocation.stackSize = kStackSize;
	outAllocation.guardSize = kGuardSize;

	return true;
}

void releaseStack(const ThreadStackAllocation& allocation) noexcept {
	void* const kReservationBase = memory::pointerMinus(allocation.stackBase, allocation.guardSize);

	memory::releaseReservation(kReservationBase, allocation.stackSize + allocation.guardSize);
}

} // namespace trivial::thread

#endif // TRIVIAL_PLATFORM_POSIX

#include <trivial/core/sync/condition_variable.h>

#include <cstdint>

#include <trivial/core/sync/mutex.h>

#include "core/sync/parking_lot.h"

namespace {

[[nodiscard]] std::uintptr_t keyFor(const void* const kAddress) noexcept {
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	return reinterpret_cast<std::uintptr_t>(kAddress);
}

} // namespace

namespace trivial::sync {

void ConditionVariable::wait(Mutex& mutex) noexcept {
	(void)activeParkingLot().park(
	    keyFor(this),
	    [] {
		    return true;
	    },
	    [&mutex] {
		    mutex.unlock();
	    });
	mutex.lock();
}

void ConditionVariable::notifyOne(Mutex& mutex) noexcept {
	(void)activeParkingLot().unparkOneRequeue(keyFor(this), keyFor(&mutex), [&mutex] {
		return mutex.markParkedIfLocked();
	});
}

void ConditionVariable::notifyAll(Mutex& mutex) noexcept {
	bool wasMutexLocked = false;
	const ParkingLot::UnparkAllRequeueResult kResult
	    = activeParkingLot().unparkAllRequeue(keyFor(this), keyFor(&mutex), [&mutex, &wasMutexLocked] {
		      wasMutexLocked = mutex.markParkedIfLocked();
		      return wasMutexLocked;
	      });

	if (!wasMutexLocked && kResult.anyRequeued) {
		mutex.markParked();
	}
}

} // namespace trivial::sync

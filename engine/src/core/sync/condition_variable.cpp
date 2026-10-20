#include <trivial/core/sync/condition_variable.h>

#include <trivial/core/sync/mutex.h>

#include "core/sync/parking_lot.h"

namespace trivial::sync {

void ConditionVariable::wait(Mutex& mutex) noexcept {
	(void)activeParkingLot().park(
	    parkingKey(*this),
	    [] {
		    return true;
	    },
	    [&mutex] {
		    mutex.unlock();
	    });
	mutex.lock();
}

void ConditionVariable::notifyOne(Mutex& mutex) noexcept {
	(void)activeParkingLot().unparkOneRequeue(parkingKey(*this), parkingKey(mutex), [&mutex] {
		return mutex.markParkedIfLocked();
	});
}

void ConditionVariable::notifyAll(Mutex& mutex) noexcept {
	bool wasMutexLocked = false;
	const ParkingLot::UnparkAllRequeueResult kResult
	    = activeParkingLot().unparkAllRequeue(parkingKey(*this), parkingKey(mutex), [&mutex, &wasMutexLocked] {
		      wasMutexLocked = mutex.markParkedIfLocked();
		      return wasMutexLocked;
	      });

	if (!wasMutexLocked && kResult.anyRequeued) {
		mutex.markParked();
	}
}

} // namespace trivial::sync

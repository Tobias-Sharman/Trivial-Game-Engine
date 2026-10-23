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
	activeParkingLot().unparkAllRequeue(
	    parkingKey(*this),
	    parkingKey(mutex),
	    [&mutex] {
		    return mutex.markParkedIfLocked();
	    },
	    [&mutex](ParkingLot::UnparkAllRequeueResult result) {
		    if (!result.requeuedAll && result.anyRequeued) {
			    mutex.markParked();
		    }
	    });
}

} // namespace trivial::sync

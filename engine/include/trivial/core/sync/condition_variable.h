#ifndef TRIVIAL_CORE_SYNC_CONDITION_VARIABLE_H
#define TRIVIAL_CORE_SYNC_CONDITION_VARIABLE_H

#include <trivial/core/sync/mutex.h>

namespace trivial::sync {

class ConditionVariable {
public:
	ConditionVariable() noexcept = default;

	~ConditionVariable() noexcept = default;

	ConditionVariable(const ConditionVariable&) = delete;
	ConditionVariable& operator=(const ConditionVariable&) = delete;

	ConditionVariable(ConditionVariable&&) = delete;
	ConditionVariable& operator=(ConditionVariable&&) = delete;

	void wait(Mutex& mutex) noexcept;

	void notifyOne(Mutex& mutex) noexcept;
	void notifyAll(Mutex& mutex) noexcept;
};

} // namespace trivial::sync

#endif // TRIVIAL_CORE_SYNC_CONDITION_VARIABLE_H

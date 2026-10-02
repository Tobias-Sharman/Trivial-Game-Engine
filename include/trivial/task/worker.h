#ifndef TRIVIAL_TASK_WORKER_H
#define TRIVIAL_TASK_WORKER_H

#include <atomic>
#include <cstddef>
#include <cstdint>

#include <trivial/core/assert.h>
#include <trivial/core/platform.h>
#include <trivial/core/thread/thread.h>
#include <trivial/task/task_priority_queue.h>

namespace trivial::task {

enum class WorkerState : std::uint8_t {
	Active,
	Waiting,
	Parked,
};

struct alignas(TRIVIAL_PLATFORM_FALSE_SHARING_ALIGNMENT) Worker {
	Worker() noexcept = default;

	~Worker() { TRIVIAL_ASSERT(!thread.joinable()); }

	Worker(const Worker&) = delete;
	Worker& operator=(const Worker&) = delete;

	Worker(Worker&&) = delete;
	Worker& operator=(Worker&&) = delete;

	std::size_t index = 0;
	std::atomic<bool> stopping{false};
	std::atomic<WorkerState> state{WorkerState::Parked};

	thread::Thread thread;

	alignas(TRIVIAL_PLATFORM_CACHE_LINE_SIZE) TaskPriorityQueue localQueue;
};

} // namespace trivial::task

#endif // TRIVIAL_TASK_WORKER_H

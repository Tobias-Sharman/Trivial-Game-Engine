#ifndef TRIVIAL_TASK_WORKER_H
#define TRIVIAL_TASK_WORKER_H

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <stop_token>
#include <thread>
#include <utility>

#include <trivial/core/assert.h>
#include <trivial/task/task_priority_queue.h>
#include <trivial/task/task_system_config.h>

namespace trivial::task {

enum class WorkerState : std::uint8_t {
	Active,
	Waiting,
	Parked
};

struct Worker {
	Worker(std::size_t workerIndex, ThreadConfig workerConfig)
	    : index(workerIndex)
	    , config(std::move(workerConfig)) {}

	~Worker() { TRIVIAL_ASSERT(!thread.joinable()); }

	Worker(const Worker&) = delete;
	Worker& operator=(const Worker&) = delete;

	Worker(Worker&&) = delete;
	Worker& operator=(Worker&&) = delete;

	std::size_t index;
	ThreadConfig config;

	std::thread thread;
	std::stop_source stopSource;

	TaskPriorityQueue localQueue;

	std::mutex stateMutex; // TODO: Custom mutex when doing the others
	std::condition_variable_any stateCv;
	WorkerState state = WorkerState::Parked; // every worker starts parked
};

} // namespace trivial::task

#endif // TRIVIAL_TASK_WORKER_H

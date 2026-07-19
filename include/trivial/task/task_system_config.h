#ifndef TRIVIAL_TASK_TASK_SYSTEM_CONFIG_H
#define TRIVIAL_TASK_TASK_SYSTEM_CONFIG_H

#include <cstddef>
#include <cstdint>
#include <string>

#include <trivial/core/log.h>
#include <trivial/task/task_launch_options.h>

namespace trivial::task {

enum class ThreadPriority : std::uint8_t {
	Low,
	Normal,
	High
};

// TODO: Custom thread type thing for support for stuff like stack size
struct ThreadConfig {
	std::string name = "Unamed worker thread";

	std::size_t stackSize = 0; // 0 -> hardware concurrency

	ThreadPriority priority = ThreadPriority::Normal;

	// TODO: Add something for cpu core selection, mask probably
};

struct WorkerConfig {
	std::uint32_t count = 0;

	std::uint32_t maxStandbyWorkers = 2;

	ThreadConfig thread{};
};

// Match task priority ordering if changing - see task_launch_options.h
struct TaskPriorityWeights {
	std::uint32_t background = 1;
	std::uint32_t normal = 2;
	std::uint32_t high = 4;
	std::uint32_t critical = 8;
};

struct TaskSchedulerConfig {
	TaskPriorityWeights priorityWeights{};

	// Unless wanting a 0 selection of a priority (will require adjusting the assertion) batchsize > sum(TaskPriorityWeights)
	std::size_t batchSize = 16;
};

struct TaskSystemConfig {
	WorkerConfig workers{};

	TaskSchedulerConfig scheduler{};

	std::uint32_t waitHelpMaxDepth = 0;

	// TODO: Config for dedicated worker threads, main, render etc.
};

} // namespace trivial::task

#endif // TRIVIAL_TASK_TASK_SYSTEM_CONFIG_H

#include <cstddef>
#include <span>
#include <vector>

#include <gtest/gtest.h>

#include <trivial/core/sync/spin_lock.h>
#include <trivial/task/task_system.h>
#include <trivial/task/task_system_config.h>

namespace {

constexpr std::size_t g_kConcurrentTasks = 8;
constexpr std::size_t g_kConcurrentIterations = 10000;
constexpr std::uint32_t g_kConcurrentWorkers = 4;

[[nodiscard]] trivial::task::TaskSystemConfig concurrentConfig() {
	trivial::task::TaskSystemConfig config;
	config.workers.count = g_kConcurrentWorkers;
	config.workers.thread.name = "Spin lock test worker";

	return config;
}

template <typename Body>
void runOnAllTasks(trivial::task::TaskSystem& taskSystem, const Body& body) {
	std::vector<trivial::task::TaskHandle> handles;
	handles.reserve(g_kConcurrentTasks);

	for (std::size_t taskIndex = 0; taskIndex < g_kConcurrentTasks; ++taskIndex) {
		handles.push_back(taskSystem.launch(trivial::task::TaskPayload{[taskIndex, &body]() noexcept {
			body(taskIndex);
		}}));
	}

	taskSystem.wait(std::span<const trivial::task::TaskHandle>{handles});

	for (trivial::task::TaskHandle handle : handles) {
		(void)taskSystem.release(handle);
	}
}

TEST(SpinLockMultiThreadTest, ConcurrentLockIncrementsNeverRace) {
	trivial::task::TaskSystem taskSystem{concurrentConfig()};
	trivial::sync::SpinLock lock;
	std::size_t counter = 0;

	runOnAllTasks(taskSystem, [&](std::size_t taskIndex) {
		(void)taskIndex;

		for (std::size_t iteration = 0; iteration < g_kConcurrentIterations; ++iteration) {
			lock.lock();
			++counter;
			lock.unlock();
		}
	});

	EXPECT_EQ(counter, g_kConcurrentTasks * g_kConcurrentIterations);
}

TEST(SpinLockMultiThreadTest, ConcurrentTryLockIncrementsNeverRace) {
	trivial::task::TaskSystem taskSystem{concurrentConfig()};
	trivial::sync::SpinLock lock;
	std::size_t counter = 0;

	runOnAllTasks(taskSystem, [&](std::size_t taskIndex) {
		(void)taskIndex;

		for (std::size_t iteration = 0; iteration < g_kConcurrentIterations; ++iteration) {
			while (!lock.tryLock()) {}

			++counter;
			lock.unlock();
		}
	});

	EXPECT_EQ(counter, g_kConcurrentTasks * g_kConcurrentIterations);
}

} // namespace

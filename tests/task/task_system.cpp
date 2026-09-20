#include <trivial/task/task_system.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <span>
#include <thread>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>

#include <trivial/core/sync/event.h>
#include <trivial/core/sync/lock_guard.h>
#include <trivial/core/sync/mutex.h>
#include <trivial/core/thread/thread.h>
#include <trivial/task/task.h>

#include "support/helpers.h"

namespace trivial::task {

namespace {

TaskSystemConfig makeResolvedTestConfig() noexcept {
	TaskSystemConfig config{};
	config.workers.count = trivial::thread::Thread::resolveConcurrency(config.workers.count);
	return config;
}

class ScopedTaskSystem {
public:
	ScopedTaskSystem() noexcept
	    : m_parkingLotScope(m_config.workers.count + m_config.workers.maxStandbyWorkers)
	    , m_taskSystem(m_config) {
		m_mainThread.adoptCurrentThread({.name = "Test Main", .type = thread::ThreadType::Main});
		setActiveTaskSystem(&m_taskSystem);
	}

	~ScopedTaskSystem() noexcept { setActiveTaskSystem(nullptr); }

	ScopedTaskSystem(const ScopedTaskSystem&) = delete;
	ScopedTaskSystem& operator=(const ScopedTaskSystem&) = delete;

	ScopedTaskSystem(ScopedTaskSystem&&) = delete;
	ScopedTaskSystem& operator=(ScopedTaskSystem&&) = delete;

private:
	TaskSystemConfig m_config = makeResolvedTestConfig();
	thread::Thread m_mainThread;
	trivial::tests::ScopedParkingLot m_parkingLotScope;
	TaskSystem m_taskSystem;
};

struct LargeTaskResult {
	std::array<std::byte, 64> storage{};
	int value = 0;
};

static_assert(sizeof(LargeTaskResult) > 40);

// ----------------------------------------------------------------------------
// Single-threaded correctness
// ----------------------------------------------------------------------------

TEST(TaskSystemTest, LaunchWithoutPrerequisites) {
	ScopedTaskSystem taskSystemScope;

	bool executed = false;

	const TaskHandle kTask = launch(TaskPayload{[&executed]() noexcept {
		executed = true;
	}});

	EXPECT_TRUE(kTask.isValid());

	wait(kTask);

	EXPECT_TRUE(executed);
	EXPECT_TRUE(isComplete(kTask));
}

TEST(TaskSystemTest, PrerequisiteRunsBeforeDependant) {
	ScopedTaskSystem taskSystemScope;

	std::vector<int> executionOrder;

	const TaskHandle kPrerequisite = launch(TaskPayload{[&executionOrder]() noexcept {
		executionOrder.push_back(1);
	}});

	const TaskHandle kDependant = launch(TaskPayload{[&executionOrder]() noexcept {
		                                     executionOrder.push_back(2);
	                                     }},
	                                     kPrerequisite);

	EXPECT_TRUE(kPrerequisite.isValid());
	EXPECT_TRUE(kDependant.isValid());

	wait(kDependant);

	ASSERT_EQ(executionOrder.size(), 2UZ);
	EXPECT_EQ(executionOrder[0], 1);
	EXPECT_EQ(executionOrder[1], 2);

	EXPECT_TRUE(isComplete(kPrerequisite));
	EXPECT_TRUE(isComplete(kDependant));
}

TEST(TaskSystemTest, MultiplePrerequisitesRunBeforeDependant) {
	ScopedTaskSystem taskSystemScope;

	bool firstExecuted = false;
	bool secondExecuted = false;
	bool dependantExecuted = false;

	const TaskHandle kFirst = launch(TaskPayload{[&firstExecuted]() noexcept {
		firstExecuted = true;
	}});

	const TaskHandle kSecond = launch(TaskPayload{[&secondExecuted]() noexcept {
		secondExecuted = true;
	}});

	const std::array<TaskHandle, 2> kPrerequisites{kFirst, kSecond};

	const TaskHandle kDependant = launch(TaskPayload{[&firstExecuted, &secondExecuted, &dependantExecuted]() noexcept {
		                                     EXPECT_TRUE(firstExecuted);
		                                     EXPECT_TRUE(secondExecuted);

		                                     dependantExecuted = true;
	                                     }},
	                                     std::span<const TaskHandle>{kPrerequisites});

	EXPECT_TRUE(kFirst.isValid());
	EXPECT_TRUE(kSecond.isValid());
	EXPECT_TRUE(kDependant.isValid());

	wait(kDependant);

	EXPECT_TRUE(firstExecuted);
	EXPECT_TRUE(secondExecuted);
	EXPECT_TRUE(dependantExecuted);

	EXPECT_TRUE(isComplete(kFirst));
	EXPECT_TRUE(isComplete(kSecond));
	EXPECT_TRUE(isComplete(kDependant));
}

TEST(TaskSystemTest, WaitOnSpanWaitsForAll) {
	ScopedTaskSystem taskSystemScope;

	bool firstExecuted = false;
	bool secondExecuted = false;

	const TaskHandle kFirst = launch(TaskPayload{[&firstExecuted]() noexcept {
		firstExecuted = true;
	}});

	const TaskHandle kSecond = launch(TaskPayload{[&secondExecuted]() noexcept {
		secondExecuted = true;
	}});

	const std::array<TaskHandle, 2> kTasks{kFirst, kSecond};

	wait(std::span<const TaskHandle>{kTasks});

	EXPECT_TRUE(firstExecuted);
	EXPECT_TRUE(secondExecuted);

	EXPECT_TRUE(isComplete(kFirst));
	EXPECT_TRUE(isComplete(kSecond));
}

TEST(TaskSystemTest, ReleaseSucceedsAfterCompletion) {
	ScopedTaskSystem taskSystemScope;

	const TaskLaunchOptions kOptions{.lifetime = TaskLifetime::Manual};

	bool executed = false;

	const TaskHandle kTask = launch(TaskPayload{[&executed]() noexcept {
		                                executed = true;
	                                }},
	                                kOptions);

	ASSERT_TRUE(kTask.isValid());

	wait(kTask);

	EXPECT_TRUE(executed);
	EXPECT_TRUE(isComplete(kTask));

	EXPECT_EQ(release(kTask), TaskReleaseResult::Success);
	EXPECT_FALSE(isComplete(kTask));
	EXPECT_EQ(release(kTask), TaskReleaseResult::InvalidHandle);
}

TEST(TaskSystemTest, ReleaseFailsBeforeCompletion) {
	ScopedTaskSystem taskSystemScope;

	sync::Event gate;
	bool executed = false;

	const TaskHandle kTask = launch(TaskPayload{[&gate, &executed]() noexcept {
		gate.wait();
		executed = true;
	}});

	ASSERT_TRUE(kTask.isValid());

	EXPECT_EQ(release(kTask), TaskReleaseResult::TaskNotComplete);
	EXPECT_FALSE(executed);

	gate.trigger();
	wait(kTask);

	EXPECT_TRUE(executed);
	EXPECT_EQ(release(kTask), TaskReleaseResult::Success);
}

TEST(TaskPriorityQueueTest, TryPopReturnsHighestPriorityFirst) {
	TaskPriorityQueue queue;

	const TaskHandle kNormalHandle{.index = 1, .generation = 0};
	const TaskHandle kCriticalHandle{.index = 2, .generation = 0};

	queue.enqueue(kNormalHandle, TaskPriority::Normal);
	queue.enqueue(kCriticalHandle, TaskPriority::Critical);

	TaskHandle popped{};

	ASSERT_TRUE(queue.tryPop(popped));
	EXPECT_EQ(popped, kCriticalHandle);

	ASSERT_TRUE(queue.tryPop(popped));
	EXPECT_EQ(popped, kNormalHandle);

	EXPECT_FALSE(queue.tryPop(popped));
}

TEST(TaskSystemTest, LaunchDeducesVoidTaskType) {
	ScopedTaskSystem taskSystemScope;

	const auto kTask = launch([]() noexcept {});

	static_assert(std::is_same_v<decltype(kTask), const Task<void>>);

	EXPECT_TRUE(kTask.isValid());

	wait(kTask);

	EXPECT_TRUE(isComplete(kTask));
}

TEST(TaskSystemTest, LaunchDeducesValueTaskType) {
	ScopedTaskSystem taskSystemScope;

	const auto kTask = launch([]() noexcept -> int {
		return 42;
	});

	static_assert(std::is_same_v<decltype(kTask), const Task<int>>);

	EXPECT_TRUE(kTask.isValid());

	wait(kTask);

	EXPECT_TRUE(isComplete(kTask));
}

TEST(TaskSystemTest, GetResultWaitsAndReturnsInline) {
	ScopedTaskSystem taskSystemScope;

	bool executed = false;

	auto task = launch([&executed]() noexcept -> int {
		executed = true;

		return 42;
	});

	static_assert(std::is_same_v<decltype(task), Task<int>>);

	EXPECT_TRUE(task.isValid());

	const int& kResult = task.getResult();

	EXPECT_TRUE(executed);
	EXPECT_TRUE(isComplete(task));
	EXPECT_EQ(kResult, 42);
}

TEST(TaskSystemTest, GetResultReturnsHeapStoredResult) {
	ScopedTaskSystem taskSystemScope;

	auto task = launch([]() noexcept -> LargeTaskResult {
		LargeTaskResult result{};
		result.value = 42;

		return result;
	});

	static_assert(std::is_same_v<decltype(task), Task<LargeTaskResult>>);

	const LargeTaskResult& kResult = task.getResult();

	EXPECT_TRUE(isComplete(task));
	EXPECT_EQ(kResult.value, 42);
}

TEST(TaskSystemTest, GetResultReturnsVector) {
	ScopedTaskSystem taskSystemScope;

	auto task = launch([]() noexcept -> std::vector<int> {
		return {1, 2, 3, 4};
	});

	static_assert(std::is_same_v<decltype(task), Task<std::vector<int>>>);

	const std::vector<int>& kResult = task.getResult();

	ASSERT_EQ(kResult.size(), 4UZ);
	EXPECT_EQ(kResult[0], 1);
	EXPECT_EQ(kResult[1], 2);
	EXPECT_EQ(kResult[2], 3);
	EXPECT_EQ(kResult[3], 4);
}

TEST(TaskSystemTest, TypedTaskCanBeUsedAsPrerequisite) {
	ScopedTaskSystem taskSystemScope;

	std::vector<int> executionOrder;

	const auto kPrerequisite = launch([&executionOrder]() noexcept -> int {
		executionOrder.push_back(1);

		return 42;
	});

	const auto kDependant = launch(
	    [&executionOrder]() noexcept {
		    executionOrder.push_back(2);
	    },
	    kPrerequisite);

	static_assert(std::is_same_v<decltype(kPrerequisite), const Task<int>>);
	static_assert(std::is_same_v<decltype(kDependant), const Task<void>>);

	wait(kDependant);

	ASSERT_EQ(executionOrder.size(), 2UZ);
	EXPECT_EQ(executionOrder[0], 1);
	EXPECT_EQ(executionOrder[1], 2);

	EXPECT_TRUE(isComplete(kPrerequisite));
	EXPECT_TRUE(isComplete(kDependant));
	EXPECT_EQ(kPrerequisite.getResult(), 42);
}

TEST(TaskSystemTest, TypedTaskExposesUnderlyingHandle) {
	ScopedTaskSystem taskSystemScope;

	const auto kTask = launch([]() noexcept -> int {
		return 42;
	});

	const TaskHandle kHandle = kTask.handle();

	EXPECT_TRUE(kHandle.isValid());
	EXPECT_EQ(kHandle, static_cast<TaskHandle>(kTask));

	wait(kHandle);

	EXPECT_TRUE(isComplete(kTask));
	EXPECT_EQ(kTask.getResult(), 42);
}

TEST(TaskSystemTest, TypedResultReleasedAfterAccess) {
	ScopedTaskSystem taskSystemScope;

	const TaskLaunchOptions kOptions{.lifetime = TaskLifetime::Manual};

	auto task = launch(
	    []() noexcept -> int {
		    return 42;
	    },
	    kOptions);

	EXPECT_EQ(task.getResult(), 42);
	EXPECT_TRUE(isComplete(task));

	EXPECT_EQ(release(task), TaskReleaseResult::Success);
	EXPECT_FALSE(isComplete(task));
}

// ----------------------------------------------------------------------------
// Multithreading
// ----------------------------------------------------------------------------

TEST(TaskSystemMultiThreadTest, IndependentTasksAllCompleteOnce) {
	ScopedTaskSystem taskSystemScope;

	constexpr std::size_t kTaskCount = 500;

	std::atomic<int> counter{0};
	std::vector<TaskHandle> handles;
	handles.reserve(kTaskCount);

	for (std::size_t i = 0; i < kTaskCount; ++i) {
		handles.push_back(launch(TaskPayload{[&counter]() noexcept {
			counter.fetch_add(1, std::memory_order_relaxed);
		}}));
	}

	for (const TaskHandle& handle : handles) {
		EXPECT_TRUE(handle.isValid());
	}

	wait(std::span<const TaskHandle>{handles});

	EXPECT_EQ(counter.load(std::memory_order_relaxed), static_cast<int>(kTaskCount));

	for (const TaskHandle& handle : handles) {
		EXPECT_TRUE(isComplete(handle));
	}
}

TEST(TaskSystemMultiThreadTest, TasksDistributeAcrossWorkers) {
	if (thread::Thread::resolveConcurrency(0) <= 1) {
		GTEST_SKIP() << "Single-core host - worker distribution cannot be observed";
	}

	ScopedTaskSystem taskSystemScope;

	constexpr std::size_t kTaskCount = 400;

	sync::Mutex workerIndexMutex;
	std::unordered_set<std::uint32_t> observedWorkerIndices;
	std::vector<TaskHandle> handles;
	handles.reserve(kTaskCount);

	for (std::size_t i = 0; i < kTaskCount; ++i) {
		handles.push_back(launch(TaskPayload{[&workerIndexMutex, &observedWorkerIndices]() noexcept {
			sync::LockGuard<sync::Mutex> lock(workerIndexMutex);
			observedWorkerIndices.insert(thread::Thread::current()->index());
		}}));
	}

	wait(std::span<const TaskHandle>{handles});

	EXPECT_GT(observedWorkerIndices.size(), 1UZ);
}

TEST(TaskSystemMultiThreadTest, DependencyChainPreservesOrder) {
	ScopedTaskSystem taskSystemScope;

	constexpr int kChainLength = 100;

	sync::Mutex orderMutex;
	std::vector<int> executionOrder;
	executionOrder.reserve(static_cast<std::size_t>(kChainLength));

	TaskHandle previous{};

	for (int i = 0; i < kChainLength; ++i) {
		TaskPayload payload{[&orderMutex, &executionOrder, i]() noexcept {
			sync::LockGuard<sync::Mutex> lock(orderMutex);
			executionOrder.push_back(i);
		}};

		previous = previous.isValid() ? launch(std::move(payload), previous) : launch(std::move(payload));
	}

	wait(previous);

	ASSERT_EQ(executionOrder.size(), static_cast<std::size_t>(kChainLength));

	for (int i = 0; i < kChainLength; ++i) {
		EXPECT_EQ(executionOrder[static_cast<std::size_t>(i)], i);
	}
}

TEST(TaskSystemMultiThreadTest, FanOutCompletesBeforeFanIn) {
	ScopedTaskSystem taskSystemScope;

	constexpr std::size_t kBranchCount = 32;

	std::atomic<std::size_t> branchesCompleted{0};
	std::vector<TaskHandle> branchHandles;
	branchHandles.reserve(kBranchCount);

	for (std::size_t i = 0; i < kBranchCount; ++i) {
		branchHandles.push_back(launch(TaskPayload{[&branchesCompleted]() noexcept {
			branchesCompleted.fetch_add(1, std::memory_order_acq_rel);
		}}));
	}

	bool joinSawAllBranchesComplete = false;

	const TaskHandle kJoin = launch(TaskPayload{[&branchesCompleted, &joinSawAllBranchesComplete]() noexcept {
		                                joinSawAllBranchesComplete
		                                    = branchesCompleted.load(std::memory_order_acquire) == kBranchCount;
	                                }},
	                                std::span<const TaskHandle>{branchHandles});

	ASSERT_TRUE(kJoin.isValid());

	wait(kJoin);

	EXPECT_EQ(branchesCompleted.load(std::memory_order_acquire), kBranchCount);
	EXPECT_TRUE(joinSawAllBranchesComplete);
}

TEST(TaskSystemMultiThreadTest, ReentrantWaitDoesNotDeadlock) {
	ScopedTaskSystem taskSystemScope;

	bool innermostExecuted = false;
	bool middleExecuted = false;
	bool outerExecuted = false;

	const TaskHandle kOuter = launch(TaskPayload{[&innermostExecuted, &middleExecuted, &outerExecuted]() noexcept {
		const TaskHandle kMiddle = launch(TaskPayload{[&innermostExecuted, &middleExecuted]() noexcept {
			const TaskHandle kInnermost = launch(TaskPayload{[&innermostExecuted]() noexcept {
				innermostExecuted = true;
			}});

			wait(kInnermost);

			middleExecuted = true;
		}});

		wait(kMiddle);

		outerExecuted = true;
	}});

	ASSERT_TRUE(kOuter.isValid());

	wait(kOuter);

	EXPECT_TRUE(innermostExecuted);
	EXPECT_TRUE(middleExecuted);
	EXPECT_TRUE(outerExecuted);
}

TEST(TaskSystemMultiThreadTest, DestructorDrainsOutstandingWork) {
	thread::Thread mainThread;
	mainThread.adoptCurrentThread({.name = "Test Main", .type = thread::ThreadType::Main});

	const TaskSystemConfig kConfig = makeResolvedTestConfig();
	trivial::tests::ScopedParkingLot parkingLotScope(kConfig.workers.count + kConfig.workers.maxStandbyWorkers);

	constexpr int kTaskCount = 20;

	std::atomic<int> completedCount{0};

	{
		TaskSystem localSystem{kConfig};

		TaskHandle previous{};

		for (int i = 0; i < kTaskCount; ++i) {
			TaskPayload payload{[&completedCount]() noexcept {
				completedCount.fetch_add(1, std::memory_order_relaxed);
			}};

			previous = previous.isValid() ? localSystem.launch(std::move(payload), previous)
			                              : localSystem.launch(std::move(payload));
		}

		// Let destructor drain
	}

	EXPECT_EQ(completedCount.load(std::memory_order_relaxed), kTaskCount);
}

TEST(TaskSystemTest, LaunchFailsWhenCapacityExhausted) {
	thread::Thread mainThread;
	mainThread.adoptCurrentThread({.name = "Test Main", .type = thread::ThreadType::Main});

	const TaskSystemConfig kConfig = makeResolvedTestConfig();
	trivial::tests::ScopedParkingLot parkingLotScope(kConfig.workers.count + kConfig.workers.maxStandbyWorkers);

	TaskSystem localSystem{kConfig};

	// NOTE: Needs adjusting if the max task count is increased
	constexpr int kMaxTaskCount = 65536;

	const TaskLaunchOptions kOptions{.lifetime = TaskLifetime::Manual};

	std::vector<TaskHandle> handles;
	handles.reserve(kMaxTaskCount + 1);

	bool sawExhaustion = false;

	for (int i = 0; i < kMaxTaskCount + 1; ++i) {
		const TaskHandle kHandle = localSystem.launch(TaskPayload{[]() noexcept {}}, kOptions);

		if (!kHandle.isValid()) {
			sawExhaustion = true;
			break;
		}

		handles.push_back(kHandle);
	}

	EXPECT_TRUE(sawExhaustion);

	localSystem.wait(std::span<const TaskHandle>{handles});

	for (TaskHandle handle : handles) {
		EXPECT_EQ(localSystem.release(handle), TaskReleaseResult::Success);
	}
}

TEST(TaskSystemMultiThreadTest, DestructorWaitsForSlowTask) {
	thread::Thread mainThread;
	mainThread.adoptCurrentThread({.name = "Test Main", .type = thread::ThreadType::Main});

	const TaskSystemConfig kConfig = makeResolvedTestConfig();
	trivial::tests::ScopedParkingLot parkingLotScope(kConfig.workers.count + kConfig.workers.maxStandbyWorkers);

	constexpr auto kTaskDuration = std::chrono::milliseconds{100};

	std::atomic<bool> taskCompleted{false};

	{
		TaskSystem localSystem{kConfig};

		const TaskHandle kTask = localSystem.launch(TaskPayload{[&taskCompleted, kTaskDuration]() noexcept {
			std::this_thread::sleep_for(kTaskDuration);
			taskCompleted.store(true, std::memory_order_release);
		}});

		ASSERT_TRUE(kTask.isValid());
	}

	EXPECT_TRUE(taskCompleted.load(std::memory_order_acquire));
}

} // namespace

} // namespace trivial::task

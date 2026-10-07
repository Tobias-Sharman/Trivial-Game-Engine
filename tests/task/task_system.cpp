#include <trivial/task/task_system.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <trivial/core/sync/event.h>
#include <trivial/core/sync/lock_guard.h>
#include <trivial/core/sync/mutex.h>
#include <trivial/core/thread/thread.h>
#include <trivial/core/time/duration.h>
#include <trivial/core/time/time.h>
#include <trivial/task/task.h>
#include <trivial/task/task_graph.h>
#include <trivial/task/task_handle.h>
#include <trivial/task/task_launch_options.h>
#include <trivial/task/task_payload.h>
#include <trivial/task/task_priority_queue.h>

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
	const ScopedTaskSystem kTaskSystemScope;

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
	const ScopedTaskSystem kTaskSystemScope;

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
	const ScopedTaskSystem kTaskSystemScope;

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
	const ScopedTaskSystem kTaskSystemScope;

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
	const ScopedTaskSystem kTaskSystemScope;

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
	const ScopedTaskSystem kTaskSystemScope;

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
	const ScopedTaskSystem kTaskSystemScope;

	const Task<void> kTask = launch([]() noexcept {});

	static_assert(std::is_same_v<decltype(kTask), const Task<void>>);

	EXPECT_TRUE(kTask.isValid());

	wait(kTask);

	EXPECT_TRUE(isComplete(kTask));
}

TEST(TaskSystemTest, LaunchDeducesValueTaskType) {
	const ScopedTaskSystem kTaskSystemScope;

	const Task<int> kTask = launch([]() noexcept -> int {
		return 42;
	});

	static_assert(std::is_same_v<decltype(kTask), const Task<int>>);

	EXPECT_TRUE(kTask.isValid());

	wait(kTask);

	EXPECT_TRUE(isComplete(kTask));
}

TEST(TaskSystemTest, GetResultWaitsAndReturnsInline) {
	const ScopedTaskSystem kTaskSystemScope;

	bool executed = false;

	const Task<int> kTask = launch([&executed]() noexcept -> int {
		executed = true;

		return 42;
	});

	static_assert(std::is_same_v<decltype(kTask), const Task<int>>);

	EXPECT_TRUE(kTask.isValid());

	const int& kResult = kTask.getResult();

	EXPECT_TRUE(executed);
	EXPECT_TRUE(isComplete(kTask));
	EXPECT_EQ(kResult, 42);
}

TEST(TaskSystemTest, GetResultReturnsHeapStoredResult) {
	const ScopedTaskSystem kTaskSystemScope;

	const Task<LargeTaskResult> kTask = launch([]() noexcept -> LargeTaskResult {
		LargeTaskResult result{};
		result.value = 42;

		return result;
	});

	static_assert(std::is_same_v<decltype(kTask), const Task<LargeTaskResult>>);

	const LargeTaskResult& kResult = kTask.getResult();

	EXPECT_TRUE(isComplete(kTask));
	EXPECT_EQ(kResult.value, 42);
}

TEST(TaskSystemTest, GetResultReturnsVector) {
	const ScopedTaskSystem kTaskSystemScope;

	const Task<std::vector<int>> kTask = launch([]() noexcept -> std::vector<int> {
		return {1, 2, 3, 4};
	});

	static_assert(std::is_same_v<decltype(kTask), const Task<std::vector<int>>>);

	const std::vector<int>& kResult = kTask.getResult();

	ASSERT_EQ(kResult.size(), 4UZ);
	EXPECT_EQ(kResult[0], 1);
	EXPECT_EQ(kResult[1], 2);
	EXPECT_EQ(kResult[2], 3);
	EXPECT_EQ(kResult[3], 4);
}

TEST(TaskSystemTest, TypedTaskCanBeUsedAsPrerequisite) {
	const ScopedTaskSystem kTaskSystemScope;

	std::vector<int> executionOrder;

	const Task<int> kPrerequisite = launch([&executionOrder]() noexcept -> int {
		executionOrder.push_back(1);

		return 42;
	});

	const Task<void> kDependant = launch(
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
	const ScopedTaskSystem kTaskSystemScope;

	const Task<int> kTask = launch([]() noexcept -> int {
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
	const ScopedTaskSystem kTaskSystemScope;

	const TaskLaunchOptions kOptions{.lifetime = TaskLifetime::Manual};

	const Task<int> kTask = launch(
	    []() noexcept -> int {
		    return 42;
	    },
	    kOptions);

	EXPECT_EQ(kTask.getResult(), 42);
	EXPECT_TRUE(isComplete(kTask));

	EXPECT_EQ(release(kTask), TaskReleaseResult::Success);
	EXPECT_FALSE(isComplete(kTask));
}

// ----------------------------------------------------------------------------
// Multithreading
// ----------------------------------------------------------------------------

TEST(TaskSystemMultiThreadTest, IndependentTasksAllCompleteOnce) {
	const ScopedTaskSystem kTaskSystemScope;

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

	const ScopedTaskSystem kTaskSystemScope;

	constexpr std::size_t kTaskCount = 2;

	std::atomic<std::size_t> started{0};
	sync::Event allStarted;
	std::array<std::uint32_t, kTaskCount> workerIndices{};
	std::array<TaskHandle, kTaskCount> handles{};

	for (std::size_t i = 0; i < kTaskCount; ++i) {
		handles[i] = launch(TaskPayload{[&started, &allStarted, &workerIndices, i]() noexcept {
			workerIndices[i] = thread::Thread::current()->index();

			if (started.fetch_add(1, std::memory_order_acq_rel) + 1 == kTaskCount) {
				allStarted.trigger();
			}

			(void)allStarted.waitFor(time::seconds(5));
		}});
	}

	EXPECT_TRUE(allStarted.waitFor(time::seconds(5)));

	wait(std::span<const TaskHandle>{handles});

	EXPECT_NE(workerIndices[0], workerIndices[1]);
}

TEST(TaskSystemMultiThreadTest, DependencyChainPreservesOrder) {
	const ScopedTaskSystem kTaskSystemScope;

	constexpr int kChainLength = 100;

	sync::Mutex orderMutex;
	std::vector<int> executionOrder;
	executionOrder.reserve(static_cast<std::size_t>(kChainLength));

	TaskHandle previous{};

	for (int i = 0; i < kChainLength; ++i) {
		TaskPayload payload{[&orderMutex, &executionOrder, i]() noexcept {
			const sync::LockGuard<sync::Mutex> kLock(orderMutex);
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
	const ScopedTaskSystem kTaskSystemScope;

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
	const ScopedTaskSystem kTaskSystemScope;

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
	const trivial::tests::ScopedParkingLot kParkingLotScope(kConfig.workers.count + kConfig.workers.maxStandbyWorkers);

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
	const trivial::tests::ScopedParkingLot kParkingLotScope(kConfig.workers.count + kConfig.workers.maxStandbyWorkers);

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

	for (const TaskHandle kHandle : handles) {
		EXPECT_EQ(localSystem.release(kHandle), TaskReleaseResult::Success);
	}
}

TEST(TaskSystemMultiThreadTest, DestructorWaitsForSlowTask) {
	thread::Thread mainThread;
	mainThread.adoptCurrentThread({.name = "Test Main", .type = thread::ThreadType::Main});

	const TaskSystemConfig kConfig = makeResolvedTestConfig();
	const trivial::tests::ScopedParkingLot kParkingLotScope(kConfig.workers.count + kConfig.workers.maxStandbyWorkers);

	constexpr time::Duration kTaskDuration = time::milliseconds(100);

	std::atomic<bool> taskCompleted{false};

	{
		TaskSystem localSystem{kConfig};

		const TaskHandle kTask = localSystem.launch(TaskPayload{[&taskCompleted, kTaskDuration]() noexcept {
			time::sleepFor(kTaskDuration);
			taskCompleted.store(true, std::memory_order_release);
		}});

		ASSERT_TRUE(kTask.isValid());
	}

	EXPECT_TRUE(taskCompleted.load(std::memory_order_acquire));
}

} // namespace

} // namespace trivial::task

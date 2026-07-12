#include <array>
#include <span>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

#include <trivial/task/task.h>

namespace trivial::task {

namespace {

TaskSystem g_taskSystem;

class TaskSystemTestSetup {
public:
	TaskSystemTestSetup() noexcept { setActiveTaskSystem(&g_taskSystem); }

	~TaskSystemTestSetup() noexcept { setActiveTaskSystem(nullptr); }

	TaskSystemTestSetup(const TaskSystemTestSetup&) = delete;
	TaskSystemTestSetup& operator=(const TaskSystemTestSetup&) = delete;

	TaskSystemTestSetup(TaskSystemTestSetup&&) = delete;
	TaskSystemTestSetup& operator=(TaskSystemTestSetup&&) = delete;
};

TaskSystemTestSetup g_taskSystemTestSetup;

struct LargeTaskResult {
	std::array<std::byte, 64> storage{};
	int value = 0;
};

static_assert(sizeof(LargeTaskResult) > 40);

TEST(TaskSystemTests, LaunchesTaskWithoutPrerequisites) {
	bool executed = false;

	const TaskHandle kTask = launch(TaskPayload{[&executed]() noexcept {
		executed = true;
	}});

	EXPECT_TRUE(kTask.isValid());
	EXPECT_FALSE(executed);
	EXPECT_FALSE(isComplete(kTask));

	wait(kTask);

	EXPECT_TRUE(executed);
	EXPECT_TRUE(isComplete(kTask));
}

TEST(TaskSystemTests, WaitExecutesSinglePrerequisiteBeforeDependant) {
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

TEST(TaskSystemTests, WaitExecutesMultiplePrerequisitesBeforeDependant) {
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

TEST(TaskSystemTests, WaitForSpanWaitsForAllTasks) {
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

TEST(TaskSystemTests, ReleaseSucceedsAfterCompletion) {
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

TEST(TaskSystemTests, ReleaseFailsBeforeCompletion) {
	bool executed = false;

	const TaskHandle kTask = launch(TaskPayload{[&executed]() noexcept {
		executed = true;
	}});

	ASSERT_TRUE(kTask.isValid());

	EXPECT_EQ(release(kTask), TaskReleaseResult::TaskNotComplete);
	EXPECT_FALSE(executed);

	wait(kTask);

	EXPECT_TRUE(executed);
	EXPECT_EQ(release(kTask), TaskReleaseResult::Success);
}

TEST(TaskSystemTests, CriticalPriorityRunsBeforeNormalPriority) {
	std::vector<int> executionOrder;

	TaskLaunchOptions normalOptions{};
	normalOptions.priority = TaskPriority::Normal;

	TaskLaunchOptions criticalOptions{};
	criticalOptions.priority = TaskPriority::Critical;

	const TaskHandle kNormal = launch(TaskPayload{[&executionOrder]() noexcept {
		                                  executionOrder.push_back(1);
	                                  }},
	                                  normalOptions);

	const TaskHandle kCritical = launch(TaskPayload{[&executionOrder]() noexcept {
		                                    executionOrder.push_back(2);
	                                    }},
	                                    criticalOptions);

	ASSERT_TRUE(kNormal.isValid());
	ASSERT_TRUE(kCritical.isValid());

	const std::array<TaskHandle, 2> kTasks{kNormal, kCritical};

	wait(std::span<const TaskHandle>{kTasks});

	ASSERT_EQ(executionOrder.size(), 2UZ);
	EXPECT_EQ(executionOrder[0], 2);
	EXPECT_EQ(executionOrder[1], 1);
}

TEST(TaskSystemTests, LaunchDeducesVoidTaskType) {
	const auto kTask = launch([]() noexcept {});

	static_assert(std::is_same_v<decltype(kTask), const Task<void>>);

	EXPECT_TRUE(kTask.isValid());

	wait(kTask);

	EXPECT_TRUE(isComplete(kTask));
}

TEST(TaskSystemTests, LaunchDeducesValueTaskType) {
	const auto kTask = launch([]() noexcept -> int {
		return 42;
	});

	static_assert(std::is_same_v<decltype(kTask), const Task<int>>);

	EXPECT_TRUE(kTask.isValid());

	wait(kTask);

	EXPECT_TRUE(isComplete(kTask));
}

TEST(TaskSystemTests, GetResultWaitsForTaskAndReturnsInlineResult) {
	bool executed = false;

	auto task = launch([&executed]() noexcept -> int {
		executed = true;

		return 42;
	});

	static_assert(std::is_same_v<decltype(task), Task<int>>);

	EXPECT_TRUE(task.isValid());
	EXPECT_FALSE(executed);
	EXPECT_FALSE(isComplete(task));

	const int& kResult = task.getResult();

	EXPECT_TRUE(executed);
	EXPECT_TRUE(isComplete(task));
	EXPECT_EQ(kResult, 42);
}

TEST(TaskSystemTests, GetResultReturnsHeapStoredResult) {
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

TEST(TaskSystemTests, GetResultReturnsVector) {
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

TEST(TaskSystemTests, TypedTaskCanBeUsedAsPrerequisite) {
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

TEST(TaskSystemTests, TypedTaskExposesUnderlyingHandle) {
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

TEST(TaskSystemTests, TypedResultTaskCanBeReleasedAfterResultAccess) {
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

} // namespace

} // namespace trivial::task

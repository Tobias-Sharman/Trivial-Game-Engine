#include <array>
#include <span>
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

TEST(TaskSystemTests, LaunchesTaskWithoutPrerequisites) {
	bool executed = false;

	const TaskHandle kTask = launch(TaskFunction{[&executed]() noexcept {
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

	const TaskHandle kPrerequisite = launch(TaskFunction{[&executionOrder]() noexcept {
		executionOrder.push_back(1);
	}});

	const TaskHandle kDependant = launch(TaskFunction{[&executionOrder]() noexcept {
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

	const TaskHandle kFirst = launch(TaskFunction{[&firstExecuted]() noexcept {
		firstExecuted = true;
	}});

	const TaskHandle kSecond = launch(TaskFunction{[&secondExecuted]() noexcept {
		secondExecuted = true;
	}});

	const std::array<TaskHandle, 2> kPrerequisites{kFirst, kSecond};

	const TaskHandle kDependant = launch(TaskFunction{[&firstExecuted, &secondExecuted, &dependantExecuted]() noexcept {
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

	const TaskHandle kFirst = launch(TaskFunction{[&firstExecuted]() noexcept {
		firstExecuted = true;
	}});

	const TaskHandle kSecond = launch(TaskFunction{[&secondExecuted]() noexcept {
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

	const TaskHandle kTask = launch(TaskFunction{[&executed]() noexcept {
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

	const TaskHandle kTask = launch(TaskFunction{[&executed]() noexcept {
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

	const TaskHandle kNormal = launch(TaskFunction{[&executionOrder]() noexcept {
		                                  executionOrder.push_back(1);
	                                  }},
	                                  normalOptions);

	const TaskHandle kCritical = launch(TaskFunction{[&executionOrder]() noexcept {
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

} // namespace

} // namespace trivial::task

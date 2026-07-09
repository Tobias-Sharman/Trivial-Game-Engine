#include <trivial/task/task_system.h>

#include <array>
#include <span>
#include <vector>

#include <gtest/gtest.h>

#include <trivial/task/task_function.h>
#include <trivial/task/task_handle.h>
#include <trivial/task/task_launch_options.h>

namespace trivial::task {

namespace {

TEST(TaskSystemTests, LaunchesTaskWithoutPrerequisites) {
	TaskSystem taskSystem;

	bool executed = false;

	const TaskHandle kTask = taskSystem.launch(TaskFunction{[&executed]() noexcept {
		executed = true;
	}});

	EXPECT_TRUE(kTask.isValid());
	EXPECT_FALSE(executed);
	EXPECT_FALSE(taskSystem.isComplete(kTask));

	taskSystem.wait(kTask);

	EXPECT_TRUE(executed);
	EXPECT_TRUE(taskSystem.isComplete(kTask));
}

TEST(TaskSystemTests, WaitExecutesSinglePrerequisiteBeforeDependant) {
	TaskSystem taskSystem;

	std::vector<int> executionOrder;

	const TaskHandle kPrerequisite = taskSystem.launch(TaskFunction{[&executionOrder]() noexcept {
		executionOrder.push_back(1);
	}});

	const TaskHandle kDependant = taskSystem.launch(TaskFunction{[&executionOrder]() noexcept {
		                                                executionOrder.push_back(2);
	                                                }},
	                                                kPrerequisite);

	EXPECT_TRUE(kPrerequisite.isValid());
	EXPECT_TRUE(kDependant.isValid());

	taskSystem.wait(kDependant);

	ASSERT_EQ(executionOrder.size(), 2UZ);
	EXPECT_EQ(executionOrder[0], 1);
	EXPECT_EQ(executionOrder[1], 2);

	EXPECT_TRUE(taskSystem.isComplete(kPrerequisite));
	EXPECT_TRUE(taskSystem.isComplete(kDependant));
}

TEST(TaskSystemTests, WaitExecutesMultiplePrerequisitesBeforeDependant) {
	TaskSystem taskSystem;

	bool firstExecuted = false;
	bool secondExecuted = false;
	bool dependantExecuted = false;

	const TaskHandle kFirst = taskSystem.launch(TaskFunction{[&firstExecuted]() noexcept {
		firstExecuted = true;
	}});

	const TaskHandle kSecond = taskSystem.launch(TaskFunction{[&secondExecuted]() noexcept {
		secondExecuted = true;
	}});

	const std::array<TaskHandle, 2> kPrerequisites{kFirst, kSecond};

	const TaskHandle kDependant
	    = taskSystem.launch(TaskFunction{[&firstExecuted, &secondExecuted, &dependantExecuted]() noexcept {
		                        EXPECT_TRUE(firstExecuted);
		                        EXPECT_TRUE(secondExecuted);

		                        dependantExecuted = true;
	                        }},
	                        std::span<const TaskHandle>{kPrerequisites});

	EXPECT_TRUE(kFirst.isValid());
	EXPECT_TRUE(kSecond.isValid());
	EXPECT_TRUE(kDependant.isValid());

	taskSystem.wait(kDependant);

	EXPECT_TRUE(firstExecuted);
	EXPECT_TRUE(secondExecuted);
	EXPECT_TRUE(dependantExecuted);

	EXPECT_TRUE(taskSystem.isComplete(kFirst));
	EXPECT_TRUE(taskSystem.isComplete(kSecond));
	EXPECT_TRUE(taskSystem.isComplete(kDependant));
}

TEST(TaskSystemTests, WaitForSpanWaitsForAllTasks) {
	TaskSystem taskSystem;

	bool firstExecuted = false;
	bool secondExecuted = false;

	const TaskHandle kFirst = taskSystem.launch(TaskFunction{[&firstExecuted]() noexcept {
		firstExecuted = true;
	}});

	const TaskHandle kSecond = taskSystem.launch(TaskFunction{[&secondExecuted]() noexcept {
		secondExecuted = true;
	}});

	const std::array<TaskHandle, 2> kTasks{kFirst, kSecond};

	taskSystem.wait(std::span<const TaskHandle>{kTasks});

	EXPECT_TRUE(firstExecuted);
	EXPECT_TRUE(secondExecuted);

	EXPECT_TRUE(taskSystem.isComplete(kFirst));
	EXPECT_TRUE(taskSystem.isComplete(kSecond));
}

TEST(TaskSystemTests, ReleaseSucceedsAfterCompletion) {
	TaskSystem taskSystem;

	const TaskLaunchOptions kOptions{.lifetime = TaskLifetime::Manual};

	bool executed = false;

	const TaskHandle kTask = taskSystem.launch(TaskFunction{[&executed]() noexcept {
		                                           executed = true;
	                                           }},
	                                           kOptions);

	ASSERT_TRUE(kTask.isValid());

	taskSystem.wait(kTask);

	EXPECT_TRUE(executed);
	EXPECT_TRUE(taskSystem.isComplete(kTask));

	EXPECT_EQ(taskSystem.release(kTask), TaskReleaseResult::Success);
	EXPECT_FALSE(taskSystem.isComplete(kTask));
	EXPECT_EQ(taskSystem.release(kTask), TaskReleaseResult::InvalidHandle);
}

TEST(TaskSystemTests, ReleaseFailsBeforeCompletion) {
	TaskSystem taskSystem;

	bool executed = false;

	const TaskHandle kTask = taskSystem.launch(TaskFunction{[&executed]() noexcept {
		executed = true;
	}});

	ASSERT_TRUE(kTask.isValid());

	EXPECT_EQ(taskSystem.release(kTask), TaskReleaseResult::TaskNotComplete);
	EXPECT_FALSE(executed);

	taskSystem.wait(kTask);

	EXPECT_TRUE(executed);
	EXPECT_EQ(taskSystem.release(kTask), TaskReleaseResult::Success);
}

TEST(TaskSystemTests, CriticalPriorityRunsBeforeNormalPriority) {
	TaskSystem taskSystem;

	std::vector<int> executionOrder;

	TaskLaunchOptions normalOptions{};
	normalOptions.priority = TaskPriority::Normal;

	TaskLaunchOptions criticalOptions{};
	criticalOptions.priority = TaskPriority::Critical;

	const TaskHandle kNormal = taskSystem.launch(TaskFunction{[&executionOrder]() noexcept {
		                                             executionOrder.push_back(1);
	                                             }},
	                                             normalOptions);

	const TaskHandle kCritical = taskSystem.launch(TaskFunction{[&executionOrder]() noexcept {
		                                               executionOrder.push_back(2);
	                                               }},
	                                               criticalOptions);

	ASSERT_TRUE(kNormal.isValid());
	ASSERT_TRUE(kCritical.isValid());

	const std::array<TaskHandle, 2> kTasks{kNormal, kCritical};

	taskSystem.wait(std::span<const TaskHandle>{kTasks});

	ASSERT_EQ(executionOrder.size(), 2UZ);
	EXPECT_EQ(executionOrder[0], 2);
	EXPECT_EQ(executionOrder[1], 1);
}

} // namespace

} // namespace trivial::task

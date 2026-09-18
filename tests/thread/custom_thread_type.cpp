#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include <trivial/core/thread/thread.h>

#include "support/helpers.h"

namespace {

void markRan(void* arg) {
	static_cast<std::atomic<bool>*>(arg)->store(true, std::memory_order_release);
}

TEST(ThreadTest, CreateJoinRunsEntry) {
	trivial::tests::requireIsolatedProcess();

	std::atomic<bool> ran{false};

	trivial::thread::Thread thread;
	trivial::thread::ThreadConfig config;
	config.name = "create-join";
#if TRIVIAL_PLATFORM_POSIX
	trivial::tests::attachStackAllocator(config);
#endif // TRIVIAL_PLATFORM_POSIX

	trivial::thread::ThreadCreateResult result = thread.create(config, &markRan, &ran);
	ASSERT_EQ(result.error, trivial::thread::ThreadCreateError::None);

	EXPECT_TRUE(thread.joinable());

	thread.join();

	EXPECT_FALSE(thread.joinable());
	EXPECT_EQ(thread.state(), trivial::thread::ThreadState::Joined);
	EXPECT_TRUE(ran.load(std::memory_order_acquire));
}

TEST(ThreadTest, SuspendedWaitsForResume) {
	trivial::tests::requireIsolatedProcess();

	std::atomic<bool> ran{false};

	trivial::thread::Thread thread;
	trivial::thread::ThreadConfig config;
	config.name = "suspended";
#if TRIVIAL_PLATFORM_POSIX
	trivial::tests::attachStackAllocator(config);
#endif // TRIVIAL_PLATFORM_POSIX
	config.createSuspended = true;

	trivial::thread::ThreadCreateResult result = thread.create(config, &markRan, &ran);
	ASSERT_EQ(result.error, trivial::thread::ThreadCreateError::None);

	EXPECT_EQ(thread.state(), trivial::thread::ThreadState::Suspended);

	std::this_thread::sleep_for(std::chrono::milliseconds(50)); // NOLINT(readability-magic-numbers)
	EXPECT_FALSE(ran.load(std::memory_order_acquire));

	thread.resume();
	thread.join();

	EXPECT_TRUE(ran.load(std::memory_order_acquire));
}

TEST(ThreadTest, IndexVisibleInEntry) {
	struct IndexObservation {
		trivial::thread::Thread* thread;
		std::atomic<std::uint32_t>* observedIndex;
	};

	trivial::tests::requireIsolatedProcess();

	std::atomic<std::uint32_t> observedIndex{0};

	trivial::thread::Thread thread;
	trivial::thread::ThreadConfig config;
	config.name = "index";
#if TRIVIAL_PLATFORM_POSIX
	trivial::tests::attachStackAllocator(config);
#endif // TRIVIAL_PLATFORM_POSIX

	IndexObservation observation{.thread = &thread, .observedIndex = &observedIndex};
	trivial::thread::ThreadCreateResult result = thread.create(
	    config,
	    [](void* arg) {
		    auto* observation = static_cast<IndexObservation*>(arg);
		    observation->observedIndex->store(observation->thread->index(), std::memory_order_release);
	    },
	    &observation);
	ASSERT_EQ(result.error, trivial::thread::ThreadCreateError::None);

	thread.join();

	EXPECT_EQ(observedIndex.load(std::memory_order_acquire), thread.index());
}

TEST(ThreadTest, IndicesAreDistinct) {
	trivial::tests::requireIsolatedProcess();

	constexpr std::size_t kThreadCount = 8;

	std::array<trivial::thread::Thread, kThreadCount> threads;
	std::vector<std::uint32_t> seenIndices;
	seenIndices.reserve(kThreadCount);

	for (trivial::thread::Thread& thread : threads) {
		trivial::thread::ThreadConfig config;
		config.name = "distinct-index";
#if TRIVIAL_PLATFORM_POSIX
		trivial::tests::attachStackAllocator(config);
#endif // TRIVIAL_PLATFORM_POSIX

		trivial::thread::ThreadCreateResult result = thread.create(config, [](void*) {}, nullptr);
		ASSERT_EQ(result.error, trivial::thread::ThreadCreateError::None);

		EXPECT_EQ(std::ranges::find(seenIndices, thread.index()), seenIndices.end());
		seenIndices.push_back(thread.index());
	}

	for (trivial::thread::Thread& thread : threads) {
		thread.join();
	}
}

TEST(ThreadTest, CurrentResolvesToSelf) {
	struct CurrentObservation {
		trivial::thread::Thread* thread;
		std::atomic<bool>* matched;
	};

	trivial::tests::requireIsolatedProcess();

	std::atomic<bool> matched{false};

	trivial::thread::Thread thread;
	trivial::thread::ThreadConfig config;
	config.name = "current";
#if TRIVIAL_PLATFORM_POSIX
	trivial::tests::attachStackAllocator(config);
#endif // TRIVIAL_PLATFORM_POSIX

	CurrentObservation observation{.thread = &thread, .matched = &matched};
	trivial::thread::ThreadCreateResult result = thread.create(
	    config,
	    [](void* arg) {
		    auto* observation = static_cast<CurrentObservation*>(arg);
		    observation->matched->store(trivial::thread::Thread::current() == observation->thread,
			                            std::memory_order_release);
	    },
	    &observation);
	ASSERT_EQ(result.error, trivial::thread::ThreadCreateError::None);

	thread.join();

	EXPECT_TRUE(matched.load(std::memory_order_acquire));
}

TEST(ThreadTest, AdoptCapturesCallingThread) {
	trivial::tests::requireIsolatedProcess();

	trivial::thread::Thread thread;
	trivial::thread::ThreadConfig config;
	config.name = "adopted-main";
	config.type = trivial::thread::ThreadType::Main;

	thread.adoptCurrentThread(config);

	EXPECT_EQ(thread.type(), trivial::thread::ThreadType::Main);
	EXPECT_EQ(thread.state(), trivial::thread::ThreadState::Running);
	EXPECT_FALSE(thread.joinable());
	EXPECT_EQ(trivial::thread::Thread::current(), &thread);
}

TEST(ThreadTest, ConcurrencyPassesThroughNonZero) {
	EXPECT_EQ(trivial::thread::Thread::resolveConcurrency(5), 5U); // NOLINT(readability-magic-numbers)
}

TEST(ThreadTest, ConcurrencyDefaultsToHardware) {
	const std::uint32_t kHardwareConcurrency = std::thread::hardware_concurrency();

	if (kHardwareConcurrency == 0U) {
		EXPECT_EQ(trivial::thread::Thread::resolveConcurrency(0), 1U);
	} else {
		EXPECT_EQ(trivial::thread::Thread::resolveConcurrency(0), kHardwareConcurrency);
	}
}

TEST(ThreadTest, DestructorAbortsIfJoinable) {
	GTEST_FLAG_SET(death_test_style, "threadsafe");

	EXPECT_DEATH(
	    {
		    trivial::thread::Thread thread;
		    trivial::thread::ThreadConfig config;
		    config.name = "leaky";
#if TRIVIAL_PLATFORM_POSIX
		    trivial::tests::attachStackAllocator(config);
#endif // TRIVIAL_PLATFORM_POSIX

		    trivial::thread::ThreadCreateResult result = thread.create(
		        config,
		        [](void*) {
			        std::this_thread::sleep_for(std::chrono::milliseconds(200));
		        }, // NOLINT(readability-magic-numbers)
		        nullptr);
		    (void)result;
	    },
#if TRIVIAL_ENABLE_LOGGING
	    "destroyed while still joinable");
#else
	    "");
#endif // TRIVIAL_ENABLE_LOGGING
}

} // namespace

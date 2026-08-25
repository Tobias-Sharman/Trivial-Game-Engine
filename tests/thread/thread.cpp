#include <trivial/core/thread/thread.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>

#include <gtest/gtest.h>

#if TRIVIAL_PLATFORM_POSIX
#include <trivial/core/thread/thread_stack_allocator.h>
#endif // TRIVIAL_PLATFORM_POSIX

namespace {

void configureForTest(trivial::thread::ThreadConfig& config) {
#if TRIVIAL_PLATFORM_POSIX
	static trivial::thread::ThreadStackAllocator s_allocator;
	config.stackAllocator = &s_allocator;
#else
	(void)config;
#endif // TRIVIAL_PLATFORM_POSIX
}

void markRan(void* arg) {
	static_cast<std::atomic<bool>*>(arg)->store(true, std::memory_order_release);
}

TEST(ThreadTest, CreateAndJoinRunsEntryAndTransitionsState) {
	std::atomic<bool> ran{false};

	trivial::thread::Thread thread;
	trivial::thread::ThreadConfig config;
	config.name = "create-join";
	configureForTest(config);

	trivial::thread::ThreadCreateResult result = thread.create(config, &markRan, &ran);
	ASSERT_EQ(result.error, trivial::thread::ThreadCreateError::None);

	EXPECT_TRUE(thread.joinable());

	thread.join();

	EXPECT_FALSE(thread.joinable());
	EXPECT_EQ(thread.state(), trivial::thread::ThreadState::Joined);
	EXPECT_TRUE(ran.load(std::memory_order_acquire));
}

TEST(ThreadTest, CreateSuspendedBlocksUntilResume) {
	std::atomic<bool> ran{false};

	trivial::thread::Thread thread;
	trivial::thread::ThreadConfig config;
	config.name = "suspended";
	configureForTest(config);
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

TEST(ThreadTest, IndexIsVisibleFromWithinTheRunningThread) {
	struct IndexObservation {
		trivial::thread::Thread* thread;
		std::atomic<std::uint32_t>* observedIndex;
	};

	std::atomic<std::uint32_t> observedIndex{0};

	trivial::thread::Thread thread;
	trivial::thread::ThreadConfig config;
	config.name = "index";
	configureForTest(config);

	IndexObservation observation{.thread = &thread, .observedIndex = &observedIndex};
	trivial::thread::ThreadCreateResult result = thread.create(
	    config,
	    [](void* arg) {
		    IndexObservation* observation = static_cast<IndexObservation*>(arg);
		    observation->observedIndex->store(observation->thread->index(), std::memory_order_release);
	    },
	    &observation);
	ASSERT_EQ(result.error, trivial::thread::ThreadCreateError::None);

	thread.join();

	EXPECT_EQ(observedIndex.load(std::memory_order_acquire), thread.index());
}

TEST(ThreadTest, DistinctThreadsGetDistinctIndices) {
	constexpr std::size_t kThreadCount = 8;

	std::array<trivial::thread::Thread, kThreadCount> threads;
	std::array<std::uint32_t, kThreadCount> seenIndices{};
	std::size_t seenCount = 0;

	for (trivial::thread::Thread& thread : threads) {
		trivial::thread::ThreadConfig config;
		config.name = "distinct-index";
		configureForTest(config);

		trivial::thread::ThreadCreateResult result = thread.create(config, [](void*) {}, nullptr);
		ASSERT_EQ(result.error, trivial::thread::ThreadCreateError::None);

		const std::uint32_t kIndex = thread.index();

		bool isDuplicate = false;
		for (std::size_t i = 0; i < seenCount; ++i) {
			if (seenIndices[i] == kIndex) { // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
				isDuplicate = true;
				break;
			}
		}
		EXPECT_FALSE(isDuplicate);

		seenIndices[seenCount] = kIndex; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
		++seenCount;
	}

	for (trivial::thread::Thread& thread : threads) {
		thread.join();
	}
}

TEST(ThreadTest, CurrentResolvesToSelfFromWithinTheRunningThread) {
	struct CurrentObservation {
		trivial::thread::Thread* thread;
		std::atomic<bool>* matched;
	};

	std::atomic<bool> matched{false};

	trivial::thread::Thread thread;
	trivial::thread::ThreadConfig config;
	config.name = "current";
	configureForTest(config);

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

TEST(ThreadTest, DestructorAbortsIfStillJoinable) {
	GTEST_FLAG_SET(death_test_style, "threadsafe");

	EXPECT_DEATH(
	    {
		    trivial::thread::Thread thread;
		    trivial::thread::ThreadConfig config;
		    config.name = "leaky";
		    configureForTest(config);

		    trivial::thread::ThreadCreateResult result = thread.create(
		        config,
		        [](void*) {
			        std::this_thread::sleep_for(std::chrono::milliseconds(200));
		        }, // NOLINT(readability-magic-numbers)
		        nullptr);
		    (void)result;
	    },
	    "destroyed while still joinable");
}

} // namespace

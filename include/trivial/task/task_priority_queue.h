#ifndef TRIVIAL_TASK_TASK_PRIORITY_QUEUE_H
#define TRIVIAL_TASK_TASK_PRIORITY_QUEUE_H

#include <algorithm>
#include <array>
#include <cstddef>
#include <deque>

#include <trivial/core/assert.h>
#include <trivial/core/sync/lock_guard.h>
#include <trivial/core/sync/mutex.h>
#include <trivial/task/task_handle.h>
#include <trivial/task/task_launch_options.h>
#include <trivial/task/task_system_config.h>

namespace trivial::task {

class TaskPriorityQueue {
public:
	TaskPriorityQueue() noexcept = default;

	~TaskPriorityQueue() noexcept = default;

	TaskPriorityQueue(const TaskPriorityQueue&) = delete;
	TaskPriorityQueue& operator=(const TaskPriorityQueue&) = delete;

	TaskPriorityQueue(TaskPriorityQueue&&) = delete;
	TaskPriorityQueue& operator=(TaskPriorityQueue&&) = delete;

	void enqueue(TaskHandle handle, TaskPriority priority) noexcept {
		TRIVIAL_ASSERT(handle.isValid());
		TRIVIAL_ASSERT(priority < TaskPriority::Count);

		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index, cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
		PriorityBucket& bucket = m_buckets[static_cast<std::size_t>(priority)];

		sync::LockGuard<sync::Mutex> lock(bucket.mutex);
		bucket.queue.push_back(handle);
	}

	[[nodiscard]] bool tryPop(TaskHandle& outHandle) noexcept {
		for (std::size_t priorityIndex = static_cast<std::size_t>(TaskPriority::Count); priorityIndex > 0;
		     --priorityIndex) {
			const std::size_t kIndex = priorityIndex - 1;

			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index, cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
			PriorityBucket& bucket = m_buckets[kIndex];

			sync::LockGuard<sync::Mutex> lock(bucket.mutex);

			if (bucket.queue.empty()) {
				continue;
			}

			outHandle = bucket.queue.front();
			bucket.queue.pop_front();

			return true;
		}

		return false;
	}

	std::size_t tryPopWeightedBatchInto(TaskPriorityQueue& destination) noexcept {
		std::size_t grantedThisCall = 0;

		for (std::size_t i = 0; i < m_buckets.size(); ++i) {
			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index, cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
			PriorityBucket& sourceBucket = m_buckets[i];
			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index, cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
			PriorityBucket& destBucket = destination.m_buckets[i];

			std::size_t kTake = 0;
			ReadyQueue taken;

			{
				sync::LockGuard<sync::Mutex> lock(sourceBucket.mutex);

				if (sourceBucket.queue.empty()) {
					continue;
				}

				// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index, cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
				kTake = std::min(kShares[i], sourceBucket.queue.size());

				for (std::size_t j = 0; j < kTake; ++j) {
					taken.push_back(sourceBucket.queue.front());
					sourceBucket.queue.pop_front();
				}
			}

			{
				sync::LockGuard<sync::Mutex> lock(destBucket.mutex);

				for (TaskHandle handle : taken) {
					destBucket.queue.push_back(handle);
				}
			}

			grantedThisCall += kTake;
		}

		return grantedThisCall;
	}

	[[nodiscard]] bool empty() const noexcept {
		for (const PriorityBucket& bucket : m_buckets) {
			sync::LockGuard<sync::Mutex> lock(bucket.mutex);

			if (!bucket.queue.empty()) {
				return false;
			}
		}

		return true;
	}

private:
	using ReadyQueue = std::deque<TaskHandle>; // TODO: custom deque

	struct PriorityBucket {
		mutable sync::Mutex mutex;
		ReadyQueue queue;
	};

	static constexpr std::array<std::size_t, static_cast<std::size_t>(TaskPriority::Count)> kShares = []() consteval {
		constexpr std::array<std::size_t, static_cast<std::size_t>(TaskPriority::Count)> kWeights{
		    TRIVIAL_TASK_PRIORITY_WEIGHT_BACKGROUND,
		    TRIVIAL_TASK_PRIORITY_WEIGHT_NORMAL,
		    TRIVIAL_TASK_PRIORITY_WEIGHT_HIGH,
		    TRIVIAL_TASK_PRIORITY_WEIGHT_CRITICAL,
		};

		std::size_t totalWeight = 0;
		for (const std::size_t kWeight : kWeights) {
			totalWeight += kWeight;
		}

		std::array<std::size_t, kWeights.size()> shares{};
		for (std::size_t i = 0; i < kWeights.size(); ++i) {
			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index, cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
			shares[i] = (std::size_t{TRIVIAL_TASK_BATCH_SIZE} * kWeights[i]) / totalWeight;
		}

		return shares;
	}();

	std::array<PriorityBucket, static_cast<std::size_t>(TaskPriority::Count)> m_buckets;
};

} // namespace trivial::task

#endif // TRIVIAL_TASK_TASK_PRIORITY_QUEUE_H

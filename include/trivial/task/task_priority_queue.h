#ifndef TRIVIAL_TASK_TASK_PRIORITY_QUEUE_H
#define TRIVIAL_TASK_TASK_PRIORITY_QUEUE_H

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>

#include <trivial/core/assert.h>
#include <trivial/core/config.h>
#include <trivial/task/task_handle.h>
#include <trivial/task/task_launch_options.h>
#include <trivial/task/task_mutex.h>
#include <trivial/task/task_system_config.h>

namespace trivial::task {

class TaskPriorityQueue {
public:
	explicit TaskPriorityQueue(const TaskSchedulerConfig& config = {}) noexcept
	    : m_weights{config.priorityWeights.background,
	                config.priorityWeights.normal,
	                config.priorityWeights.high,
	                config.priorityWeights.critical}
	    , m_totalWeight(m_weights[0] + m_weights[1] + m_weights[2] + m_weights[3])
	    , m_shares(computeShares(m_weights, m_totalWeight, config.batchSize)) {

#if TRIVIAL_CONFIG_DEBUG
		for (std::uint32_t weight : m_weights) {
			TRIVIAL_ASSERT(weight > 0);
		}

		TRIVIAL_ASSERT(config.batchSize > 0);
#endif // TRIVIAL_CONFIG_DEBUG
	}

	~TaskPriorityQueue() noexcept = default;

	TaskPriorityQueue(const TaskPriorityQueue&) = delete;
	TaskPriorityQueue& operator=(const TaskPriorityQueue&) = delete;

	TaskPriorityQueue(TaskPriorityQueue&&) = delete;
	TaskPriorityQueue& operator=(TaskPriorityQueue&&) = delete;

	void enqueue(TaskHandle handle, TaskPriority priority) noexcept {
		TRIVIAL_ASSERT(handle.isValid());
		TRIVIAL_ASSERT(priority < TaskPriority::Count);

		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
		PriorityBucket& bucket = m_buckets[static_cast<std::size_t>(priority)];

		std::lock_guard<TaskGraphMutex> lock(bucket.mutex);
		bucket.queue.push_back(handle);
	}

	[[nodiscard]] bool tryPop(TaskHandle& outHandle) noexcept {
		for (auto priorityIndex = static_cast<std::size_t>(TaskPriority::Count); priorityIndex > 0; --priorityIndex) {
			const std::size_t kIndex = priorityIndex - 1;

			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
			PriorityBucket& bucket = m_buckets[kIndex];

			std::lock_guard<TaskGraphMutex> lock(bucket.mutex);

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
			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
			PriorityBucket& sourceBucket = m_buckets[i];
			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
			PriorityBucket& destBucket = destination.m_buckets[i];

			std::scoped_lock lock(sourceBucket.mutex, destBucket.mutex);

			if (sourceBucket.queue.empty()) {
				continue;
			}

			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
			const std::size_t kTake = std::min(m_shares[i], sourceBucket.queue.size());

			for (std::size_t j = 0; j < kTake; ++j) {
				destBucket.queue.push_back(sourceBucket.queue.front());
				sourceBucket.queue.pop_front();
			}

			grantedThisCall += kTake;
		}

		return grantedThisCall;
	}

	[[nodiscard]] bool empty() const noexcept {
		for (const PriorityBucket& bucket : m_buckets) {
			std::lock_guard<TaskGraphMutex> lock(bucket.mutex);

			if (!bucket.queue.empty()) {
				return false;
			}
		}

		return true;
	}

private:
	using ReadyQueue = std::deque<TaskHandle>; // TODO: custom deque

	struct PriorityBucket {
		mutable TaskGraphMutex mutex;
		ReadyQueue queue;
	};

	[[nodiscard]] static std::array<std::size_t, static_cast<std::size_t>(TaskPriority::Count)> computeShares(
	    const std::array<std::uint32_t, static_cast<std::size_t>(TaskPriority::Count)>& weights,
	    std::uint32_t totalWeight,
	    std::size_t batchSize) noexcept {
		std::array<std::size_t, static_cast<std::size_t>(TaskPriority::Count)> shares{};

		for (std::size_t i = 0; i < weights.size(); ++i) {
			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
			shares[i] = (batchSize * static_cast<std::size_t>(weights[i])) / totalWeight;
			TRIVIAL_ASSERT(shares[i] > 0); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
		}

		return shares;
	}

	std::array<PriorityBucket, static_cast<std::size_t>(TaskPriority::Count)> m_buckets;

	std::array<std::uint32_t, static_cast<std::size_t>(TaskPriority::Count)> m_weights;
	std::uint32_t m_totalWeight;

	std::array<std::size_t, static_cast<std::size_t>(TaskPriority::Count)> m_shares;
};

} // namespace trivial::task

#endif // TRIVIAL_TASK_TASK_PRIORITY_QUEUE_H

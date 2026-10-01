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

		PriorityBucket& bucket = m_buckets[static_cast<std::size_t>(priority)];

		const sync::LockGuard<sync::Mutex> kLock(bucket.mutex);
		bucket.queue.push_back(handle);
	}

	[[nodiscard]] bool tryPop(TaskHandle& outHandle) noexcept {
		for (std::size_t priorityIndex = static_cast<std::size_t>(TaskPriority::Count); priorityIndex > 0;
		     --priorityIndex) {
			const std::size_t kIndex = priorityIndex - 1;

			PriorityBucket& bucket = m_buckets[kIndex];

			const sync::LockGuard<sync::Mutex> kLock(bucket.mutex);

			if (bucket.queue.empty()) {
				continue;
			}

			outHandle = bucket.queue.front();
			bucket.queue.pop_front();

			return true;
		}

		return false;
	}

	[[nodiscard]] std::size_t tryPopWeightedBatchInto(TaskPriorityQueue& destination) noexcept {
		static constexpr std::array<std::size_t, static_cast<std::size_t>(TaskPriority::Count)> s_kShares{
		    TRIVIAL_TASK_PRIORITY_SHARE_BACKGROUND,
		    TRIVIAL_TASK_PRIORITY_SHARE_NORMAL,
		    TRIVIAL_TASK_PRIORITY_SHARE_HIGH,
		    TRIVIAL_TASK_PRIORITY_SHARE_CRITICAL,
		};

		std::size_t grantedThisCall = 0;

		for (std::size_t i = 0; i < m_buckets.size(); ++i) {
			PriorityBucket& sourceBucket = m_buckets[i];
			PriorityBucket& destBucket = destination.m_buckets[i];

			std::size_t take = 0;
			ReadyQueue taken;

			{
				const sync::LockGuard<sync::Mutex> kLock(sourceBucket.mutex);

				if (sourceBucket.queue.empty()) {
					continue;
				}

				take = std::min(s_kShares[i], sourceBucket.queue.size());

				for (std::size_t j = 0; j < take; ++j) {
					taken.push_back(sourceBucket.queue.front());
					sourceBucket.queue.pop_front();
				}
			}

			{
				const sync::LockGuard<sync::Mutex> kLock(destBucket.mutex);

				for (const TaskHandle kHandle : taken) {
					destBucket.queue.push_back(kHandle);
				}
			}

			grantedThisCall += take;
		}

		return grantedThisCall;
	}

	[[nodiscard]] bool empty() const noexcept {
		for (const PriorityBucket& bucket : m_buckets) {
			const sync::LockGuard<sync::Mutex> kLock(bucket.mutex);

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

	std::array<PriorityBucket, static_cast<std::size_t>(TaskPriority::Count)> m_buckets;
};

} // namespace trivial::task

#endif // TRIVIAL_TASK_TASK_PRIORITY_QUEUE_H

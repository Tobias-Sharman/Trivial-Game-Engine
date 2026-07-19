#ifndef TRIVIAL_TASK_TASK_WAIT_GROUP_H
#define TRIVIAL_TASK_TASK_WAIT_GROUP_H

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <stop_token>

namespace trivial::task {

class TaskWaitGroup {
public:
	explicit TaskWaitGroup(std::size_t count) noexcept
	    : m_remaining(count) {}

	~TaskWaitGroup() noexcept = default;

	TaskWaitGroup(const TaskWaitGroup&) = delete;
	TaskWaitGroup& operator=(const TaskWaitGroup&) = delete;

	TaskWaitGroup(TaskWaitGroup&&) = delete;
	TaskWaitGroup& operator=(TaskWaitGroup&&) = delete;

	static void onMemberComplete(TaskWaitGroup* group) noexcept {
		if (group->m_remaining.fetch_sub(1, std::memory_order_acq_rel) == 1) {
			{
				std::lock_guard<std::mutex> lock(group->m_mutex);
				group->m_signaled = true;
			}

			group->m_cv.notify_all();
		}
	}

	void wait() noexcept {
		std::unique_lock<std::mutex> lock(m_mutex);
		m_cv.wait(lock, [this] {
			return m_signaled;
		});
	}

	[[nodiscard]] bool wait(const std::stop_token& stopToken) noexcept {
		std::unique_lock<std::mutex> lock(m_mutex);
		return m_cv.wait(lock, stopToken, [this] {
			return m_signaled;
		});
	}

private:
	std::atomic<std::size_t> m_remaining;

	std::mutex m_mutex;
	std::condition_variable_any m_cv;
	bool m_signaled = false;
};

} // namespace trivial::task

#endif // TRIVIAL_TASK_TASK_WAIT_GROUP_H

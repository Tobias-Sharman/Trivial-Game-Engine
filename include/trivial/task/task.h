#ifndef TRIVIAL_TASK_TASK_H
#define TRIVIAL_TASK_TASK_H

#include <span>
#include <type_traits>
#include <utility>

#include <trivial/core/assert.h>
#include <trivial/task/task_handle.h>
#include <trivial/task/task_launch_options.h>
#include <trivial/task/task_payload.h>
#include <trivial/task/task_system.h>

namespace trivial::task {

namespace detail {

// Could have put in cpp and that would be safer and more "correct" but this will guarante less overhead rather than
// maybe not have some overhead with compiler removing wrappers and inling stuff
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
inline TaskSystem* g_activeTaskSystem = nullptr;

} // namespace detail

inline void setActiveTaskSystem(TaskSystem* taskSystem) noexcept {
	detail::g_activeTaskSystem = taskSystem;
}

[[nodiscard]] inline TaskSystem& activeTaskSystem() noexcept {
	TRIVIAL_ASSERT(detail::g_activeTaskSystem != nullptr);

	return *detail::g_activeTaskSystem;
}

template <typename Result>
class Task {
public:
	Task() noexcept = default;

	explicit constexpr Task(TaskHandle handle) noexcept
	    : m_handle(handle) {}

	[[nodiscard]] constexpr bool isValid() const noexcept { return m_handle.isValid(); }

	[[nodiscard]] constexpr TaskHandle handle() const noexcept { return m_handle; }

	[[nodiscard]] constexpr operator TaskHandle() const noexcept { return m_handle; }

	[[nodiscard]] Result& getResult() const noexcept {
		return *static_cast<Result*>(activeTaskSystem().getResultPointer(m_handle));
	}

private:
	TaskHandle m_handle{};
};

template <>
class Task<void> {
public:
	Task() noexcept = default;

	explicit constexpr Task(TaskHandle handle) noexcept
	    : m_handle(handle) {}

	[[nodiscard]] constexpr bool isValid() const noexcept { return m_handle.isValid(); }

	[[nodiscard]] constexpr TaskHandle handle() const noexcept { return m_handle; }

	[[nodiscard]] constexpr operator TaskHandle() const noexcept { return m_handle; }

private:
	TaskHandle m_handle{};
};

static_assert(sizeof(Task<void>) == sizeof(TaskHandle));
static_assert(sizeof(Task<int>) == sizeof(TaskHandle));

[[nodiscard]] inline TaskHandle launch(TaskPayload payload, const TaskLaunchOptions& options = {}) noexcept {
	return activeTaskSystem().launch(std::move(payload), options);
}

[[nodiscard]] inline TaskHandle launch(TaskPayload payload,
                                       TaskHandle prerequisite,
                                       const TaskLaunchOptions& options = {}) noexcept {
	return activeTaskSystem().launch(std::move(payload), prerequisite, options);
}

[[nodiscard]] inline TaskHandle launch(TaskPayload payload,
                                       std::span<const TaskHandle> prerequisites,
                                       const TaskLaunchOptions& options = {}) noexcept {
	return activeTaskSystem().launch(std::move(payload), prerequisites, options);
}

template <typename Callable>
[[nodiscard]] auto launch(Callable&& callable, const TaskLaunchOptions& options = {}) noexcept {
	using StoredCallable = std::decay_t<Callable>;
	using Result = std::invoke_result_t<StoredCallable&>;

	static_assert(!std::is_reference_v<Result>, "Task reference results are not currently supported");

	TaskHandle handle = launch(TaskPayload{std::forward<Callable>(callable)}, options);

	return Task<Result>{handle};
}

template <typename Callable>
[[nodiscard]] auto launch(Callable&& callable,
                          TaskHandle prerequisite,
                          const TaskLaunchOptions& options = {}) noexcept {
	using StoredCallable = std::decay_t<Callable>;
	using Result = std::invoke_result_t<StoredCallable&>;

	static_assert(!std::is_reference_v<Result>, "Task reference results are not currently supported");

	TaskHandle handle = launch(TaskPayload{std::forward<Callable>(callable)}, prerequisite, options);

	return Task<Result>{handle};
}

template <typename Callable>
[[nodiscard]] auto launch(Callable&& callable,
                          std::span<const TaskHandle> prerequisites,
                          const TaskLaunchOptions& options = {}) noexcept {
	using StoredCallable = std::decay_t<Callable>;
	using Result = std::invoke_result_t<StoredCallable&>;

	static_assert(!std::is_reference_v<Result>, "Task reference results are not currently supported");

	TaskHandle handle = launch(TaskPayload{std::forward<Callable>(callable)}, prerequisites, options);

	return Task<Result>{handle};
}

inline void wait(TaskHandle task) noexcept {
	activeTaskSystem().wait(task);
}

inline void wait(std::span<const TaskHandle> tasks) noexcept {
	activeTaskSystem().wait(tasks);
}

[[nodiscard]] inline bool isComplete(TaskHandle task) noexcept {
	return activeTaskSystem().isComplete(task);
}

[[nodiscard]] inline TaskReleaseResult release(TaskHandle task) noexcept {
	return activeTaskSystem().release(task);
}

} // namespace trivial::task

#endif // TRIVIAL_TASK_TASK_H

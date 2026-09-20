#ifndef TRIVIAL_CORE_THREAD_THREAD_H
#define TRIVIAL_CORE_THREAD_THREAD_H

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

#include <trivial/core/platform.h>
#include <trivial/core/thread/thread_config.h>

#if TRIVIAL_PLATFORM_POSIX
#include <trivial/core/thread/thread_stack_allocator.h>
#endif // TRIVIAL_PLATFORM_POSIX

#if TRIVIAL_PLATFORM_MACOS
#include <pthread/qos.h>
#endif // TRIVIAL_PLATFORM_MACOS

namespace trivial::thread {

enum class ThreadType : std::uint8_t {
	Unknown,
	Main,
	Worker,
	Render,
	Background,
};

enum class ThreadState : std::uint8_t {
	NotStarted,
	Suspended,
	Running,
	Joined,
};

enum class ThreadCreateError : std::uint8_t {
	None,
	InvalidConfig,
	StackAllocationFailed,
	ResourceLimitReached,
	OutOfMemory,
	PlatformError,
};

struct ThreadCreateResult {
	ThreadCreateError error = ThreadCreateError::None;
	int platformErrorCode = 0;
};

using ThreadStartRoutine = void (*)(void* arg);

struct NativeHandleStorage {
private:
	[[maybe_unused]] std::uint64_t m_raw = 0;
};

struct ThreadConfig {
	const char* name = nullptr;
	ThreadType type = ThreadType::Unknown;
	std::size_t stackSize = TRIVIAL_THREAD_DEFAULT_STACK_SIZE_BYTES;
	bool createSuspended = false;

#if TRIVIAL_PLATFORM_LINUX
	int schedPolicy = 0; // SCHED_OTHER
	int schedPriority = 0;
#elif TRIVIAL_PLATFORM_WINDOWS
	int win32Priority = 0;
#elif TRIVIAL_PLATFORM_MACOS
	qos_class_t qosClass = QOS_CLASS_DEFAULT;
	int qosRelativePriority = 0;
#endif // Priority representation

#if TRIVIAL_PLATFORM_HAS_CPU_AFFINITY
	std::uint64_t affinityMask = 0;
	std::uint16_t affinityGroup = 0;
#endif // TRIVIAL_PLATFORM_HAS_CPU_AFFINITY

#if TRIVIAL_PLATFORM_POSIX
	ThreadStackAllocator* stackAllocator = nullptr;
#endif // TRIVIAL_PLATFORM_POSIX
};

class Thread {
public:
	static constexpr std::size_t kMaxNameLength = 16;

	Thread() noexcept = default;

	~Thread() noexcept;

	Thread(const Thread&) = delete;
	Thread& operator=(const Thread&) = delete;

	Thread(Thread&&) = delete;
	Thread& operator=(Thread&&) = delete;

	[[nodiscard]] ThreadCreateResult create(const ThreadConfig& config,
	                                        ThreadStartRoutine startRoutine,
	                                        void* arg) noexcept;

	void adoptCurrentThread(const ThreadConfig& config) noexcept;

	void rename(const char* name) noexcept;

	void resume() noexcept;

	void join() noexcept;

	// Could not find reason to keep detach() so removed it. There was resultant
	// issues with stack reclamation so with no good reason for use with
	// comparison to using join with a higher level manager it was dropped
	//
	// No tracking in git so don't bother checking if needing in the future

	[[nodiscard]] bool joinable() const noexcept {
		if (m_type == ThreadType::Main) {
			return false;
		}

		const ThreadState kState = m_state.load(std::memory_order_acquire);
		return kState == ThreadState::Running || kState == ThreadState::Suspended;
	}
	[[nodiscard]] std::uint32_t index() const noexcept { return m_index; }
	[[nodiscard]] const char* name() const noexcept { return m_name.data(); }
	[[nodiscard]] ThreadType type() const noexcept { return m_type; }
	[[nodiscard]] ThreadState state() const noexcept { return m_state.load(std::memory_order_acquire); }
	[[nodiscard]] static Thread* current() noexcept;

	static void yield() noexcept;

	// requested == 0 resolves to std::thread::hardware_concurrency() (or 1 if
	// that can't be determined) any other value passes through unchanged
	[[nodiscard]] static std::uint32_t resolveConcurrency(std::uint32_t requested) noexcept;

private:
	static void runEntry(Thread* self) noexcept;

#if TRIVIAL_PLATFORM_POSIX
	static void* posixThreadEntry(void* arg) noexcept;
#endif // TRIVIAL_PLATFORM_POSIX

#if TRIVIAL_PLATFORM_WINDOWS
	static unsigned long __stdcall win32ThreadEntry(void* arg) noexcept;
#endif // TRIVIAL_PLATFORM_WINDOWS

	std::uint32_t m_index = 0;

	ThreadType m_type = ThreadType::Unknown;
	std::atomic<ThreadState> m_state{ThreadState::NotStarted};

	std::array<char, kMaxNameLength> m_name{};

	ThreadStartRoutine m_startRoutine = nullptr;
	void* m_arg = nullptr;

#if TRIVIAL_PLATFORM_POSIX
	ThreadStackAllocator* m_stackAllocator = nullptr;
	ThreadStackAllocation m_stackAllocation{};
#endif // TRIVIAL_PLATFORM_POSIX

#if TRIVIAL_PLATFORM_MACOS
	qos_class_t m_qosClass = QOS_CLASS_DEFAULT;
	int m_qosRelativePriority = 0;
#endif // TRIVIAL_PLATFORM_MACOS

	NativeHandleStorage m_nativeHandleStorage{};
};

} // namespace trivial::thread

#endif // TRIVIAL_CORE_THREAD_THREAD_H

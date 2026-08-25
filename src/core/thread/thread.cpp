#include <trivial/core/thread/thread.h>

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <cstring>

#include <trivial/core/assert.h>
#include <trivial/core/log.h>

#if TRIVIAL_PLATFORM_POSIX
#include <cerrno>
#include <pthread.h>
#include <sched.h>
#endif // TRIVIAL_PLATFORM_POSIX

#if TRIVIAL_PLATFORM_MACOS
#include <mach/mach.h>
#include <mach/thread_switch.h>
#endif // TRIVIAL_PLATFORM_MACOS

#if TRIVIAL_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // TRIVIAL_PLATFORM_WINDOWS

namespace {

// Non-atomic would be safe but cost is much less than the syscalls for thread creation and makes it safer for future
// planned usage changes
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::atomic<std::uint32_t> g_nextThreadIndex{0};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
thread_local trivial::thread::Thread* g_currentThread = nullptr; // Better linkage than member variable

void copyName(const char* name, std::array<char, trivial::thread::Thread::kMaxNameLength>& outName) noexcept {
	if (name == nullptr) {
		outName[0] = '\0';
		return;
	}

	const std::size_t kLength = std::min(std::strlen(name), outName.size() - 1);

	std::memcpy(outName.data(), name, kLength);
	outName[kLength] = '\0'; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
}

} // namespace

namespace trivial::thread {

Thread::~Thread() noexcept {
	if (joinable()) [[unlikely]] {
		TRIVIAL_LOG_FATAL_PREFIX("Thread", "destroyed while still joinable - call join() first");
		std::abort();
	}
}

[[nodiscard]] ThreadCreateResult Thread::create(const ThreadConfig& config,
                                                ThreadStartRoutine startRoutine,
                                                void* arg) noexcept {
	TRIVIAL_ASSERT(m_state.load(std::memory_order_relaxed) == ThreadState::NotStarted);
	TRIVIAL_ASSERT(startRoutine != nullptr);

	m_index = g_nextThreadIndex.fetch_add(1, std::memory_order_relaxed);
	m_type = config.type;
	m_startRoutine = startRoutine;
	m_arg = arg;

	copyName(config.name, m_name);

#if TRIVIAL_PLATFORM_MACOS
	m_qosClass = config.qosClass;
	m_qosRelativePriority = config.qosRelativePriority;
#endif // TRIVIAL_PLATFORM_MACOS

#if TRIVIAL_PLATFORM_POSIX
	TRIVIAL_ASSERT(config.stackAllocator != nullptr);

	ThreadStackAllocation allocation{};
	int osErrorCode = 0;

	if (!config.stackAllocator->allocate(config.stackSize, allocation, osErrorCode)) {
		return ThreadCreateResult{.error = ThreadCreateError::StackAllocationFailed, .platformErrorCode = osErrorCode};
	}

	m_stackAllocator = config.stackAllocator;
	m_stackAllocation = allocation;

	pthread_attr_t attr{};
	pthread_attr_init(&attr);
	pthread_attr_setstack(&attr, allocation.stackBase, allocation.stackSize);

#if TRIVIAL_PLATFORM_LINUX
	if (config.schedPolicy != SCHED_OTHER) {
		pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
		pthread_attr_setschedpolicy(&attr, config.schedPolicy);

		sched_param param{};
		param.sched_priority = config.schedPriority;
		pthread_attr_setschedparam(&attr, &param);
	}

	cpu_set_t cpuSet{};

	if (config.affinityMask != 0) {
		CPU_ZERO(&cpuSet);

		for (unsigned int bit = 0; bit < 64; ++bit) {
			if ((config.affinityMask & (std::uint64_t{1} << bit)) != 0) {
				CPU_SET((config.affinityGroup * 64) + bit, &cpuSet);
			}
		}

		pthread_attr_setaffinity_np(&attr, sizeof(cpuSet), &cpuSet);
	}
#endif // TRIVIAL_PLATFORM_LINUX

	m_state.store(config.createSuspended ? ThreadState::Suspended : ThreadState::Running, std::memory_order_release);

	pthread_t handle{};
	const int kResult = pthread_create(&handle, &attr, &Thread::posixThreadEntry, this);

	pthread_attr_destroy(&attr);

	if (kResult != 0) {
		m_stackAllocator->release(m_stackAllocation);
		m_stackAllocator = nullptr;
		m_stackAllocation = ThreadStackAllocation{};
		m_state.store(ThreadState::NotStarted, std::memory_order_release);

		ThreadCreateError error = ThreadCreateError::PlatformError;
		if (kResult == EAGAIN) {
			error = ThreadCreateError::ResourceLimitReached;
		} else if (kResult == ENOMEM) {
			error = ThreadCreateError::OutOfMemory;
		}

		return ThreadCreateResult{.error = error, .platformErrorCode = kResult};
	}

	std::memcpy(m_nativeHandleStorage.data(), static_cast<const void*>(&handle), sizeof(pthread_t));

#if TRIVIAL_PLATFORM_LINUX
	if (m_name[0] != '\0') {
		pthread_setname_np(handle, m_name.data());
	}
#endif // TRIVIAL_PLATFORM_LINUX

#elif TRIVIAL_PLATFORM_WINDOWS
	HANDLE handle = CreateThread(nullptr, config.stackSize, &Thread::win32ThreadEntry, this, CREATE_SUSPENDED, nullptr);

	if (handle == nullptr) {
		const DWORD kError = GetLastError();

		ThreadCreateError error = ThreadCreateError::PlatformError;
		if (kError == ERROR_NOT_ENOUGH_MEMORY || kError == ERROR_OUTOFMEMORY) {
			error = ThreadCreateError::OutOfMemory;
		}

		return ThreadCreateResult{.error = error, .platformErrorCode = static_cast<int>(kError)};
	}

	SetThreadPriority(handle, config.win32Priority);

#if TRIVIAL_PLATFORM_HAS_CPU_AFFINITY
	if (config.affinityMask != 0) {
		GROUP_AFFINITY groupAffinity{};
		groupAffinity.Mask = config.affinityMask;
		groupAffinity.Group = config.affinityGroup;

		SetThreadGroupAffinity(handle, &groupAffinity, nullptr);
	}
#endif // TRIVIAL_PLATFORM_HAS_CPU_AFFINITY

	m_state.store(config.createSuspended ? ThreadState::Suspended : ThreadState::Running, std::memory_order_release);

	std::memcpy(m_nativeHandleStorage.data(), static_cast<const void*>(&handle), sizeof(HANDLE));

	ResumeThread(handle);
#endif // Platform-specific creation

	return ThreadCreateResult{.error = ThreadCreateError::None, .platformErrorCode = 0};
}

void Thread::resume() noexcept {
	TRIVIAL_ASSERT(m_state.load(std::memory_order_relaxed) == ThreadState::Suspended);

	m_state.store(ThreadState::Running, std::memory_order_release);
	m_state.notify_one();
}

void Thread::join() noexcept {
	TRIVIAL_ASSERT(joinable());

#if TRIVIAL_PLATFORM_POSIX
	pthread_t handle{};
#elif TRIVIAL_PLATFORM_WINDOWS
	HANDLE handle{};
#endif // Native handle type

	std::memcpy(static_cast<void*>(&handle), m_nativeHandleStorage.data(), sizeof(decltype(handle)));

#if TRIVIAL_PLATFORM_POSIX
	pthread_join(handle, nullptr);

	m_stackAllocator->release(m_stackAllocation);
	m_stackAllocator = nullptr;
	m_stackAllocation = ThreadStackAllocation{};
#elif TRIVIAL_PLATFORM_WINDOWS
	WaitForSingleObject(handle, INFINITE);
	CloseHandle(handle);
#endif // Platform-specific join

	m_state.store(ThreadState::Joined, std::memory_order_release);
}

[[nodiscard]] Thread* Thread::current() noexcept {
	return g_currentThread;
}

// TODO: Change with custom wait primitives to directly yield to waiting thread
//       if possible
//       For Macos adjust MACH_PORT_NULL to thread wanted and option ought to
//       not change
void Thread::yield() noexcept {
#if TRIVIAL_PLATFORM_MACOS
	thread_switch(MACH_PORT_NULL, SWITCH_OPTION_DEPRESS, TRIVIAL_THREAD_MACOS_YIELD_DEPRESS_TIMEOUT_MS);

#elif TRIVIAL_PLATFORM_LINUX
	sched_yield();

#elif TRIVIAL_PLATFORM_WINDOWS
#if TRIVIAL_THREAD_WINDOWS_YIELD_USE_SLEEP_ZERO
	Sleep(0);
#else
	SwitchToThread();
#endif // TRIVIAL_THREAD_WINDOWS_YIELD_USE_SLEEP_ZERO

#endif // Platform-specific yield
}

void Thread::runEntry(Thread* self) noexcept {
	g_currentThread = self;

	if (self->m_state.load(std::memory_order_acquire) == ThreadState::Suspended) {
		self->m_state.wait(ThreadState::Suspended, std::memory_order_acquire);
	}

#if TRIVIAL_PLATFORM_MACOS
	pthread_set_qos_class_self_np(self->m_qosClass, self->m_qosRelativePriority);

	if (self->m_name[0] != '\0') {
		pthread_setname_np(self->m_name.data());
	}
#endif // TRIVIAL_PLATFORM_MACOS

	self->m_startRoutine(self->m_arg); // NOTE: Should surface errors
}

#if TRIVIAL_PLATFORM_POSIX

void* Thread::posixThreadEntry(void* arg) noexcept {
	runEntry(static_cast<Thread*>(arg));
	return nullptr;
}

#endif // TRIVIAL_PLATFORM_POSIX

#if TRIVIAL_PLATFORM_WINDOWS

unsigned long __stdcall Thread::win32ThreadEntry(void* arg) noexcept {
	runEntry(static_cast<Thread*>(arg));
	return 0;
}

#endif // TRIVIAL_PLATFORM_WINDOWS

} // namespace trivial::thread

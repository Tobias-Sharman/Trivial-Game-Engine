#include <trivial/core/thread/thread.h>

#include <algorithm>
#include <atomic>
#include <bit>
#include <cstdlib>
#include <cstring>
#include <thread>

#include <trivial/core/assert.h>
#include <trivial/core/log.h>

#if TRIVIAL_PLATFORM_POSIX
#include <cerrno>
#include <pthread.h>
#include <sched.h>

static_assert(sizeof(pthread_t) == sizeof(void*), "pthread_t is no longer NativeHandleStorage-sized");
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

// Non-atomic would be safe but cost is much less than the syscalls for thread
// creation and makes it safer for future planned usage changes
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::atomic<std::uint32_t> g_nextThreadIndex{0};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
thread_local trivial::thread::Thread* g_currentThread = nullptr; // Better linkage than member variable

void copyName(const char* name, std::array<char, trivial::thread::Thread::kMaxNameLength>& outName) noexcept {
	if (name == nullptr) {
		outName[0] = '\0'; // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
		return;
	}

	const std::size_t kLength = std::min(std::strlen(name), outName.size() - 1);

	std::memcpy(outName.data(), name, kLength);
	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
	outName[kLength] = '\0';
}

#if TRIVIAL_PLATFORM_WINDOWS
void applyThreadDescription(HANDLE handle,
                            const std::array<char, trivial::thread::Thread::kMaxNameLength>& name) noexcept {
	if (name[0] == '\0') { // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
		return;
	}

	std::array<wchar_t, trivial::thread::Thread::kMaxNameLength> wideName{};

	std::size_t i = 0;
	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
	for (; i < name.size() - 1 && name[i] != '\0'; ++i) {
		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
		wideName[i] = static_cast<wchar_t>(static_cast<unsigned char>(name[i]));
	}
	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
	wideName[i] = L'\0';

	if (FAILED(SetThreadDescription(handle, wideName.data()))) {
		TRIVIAL_LOG_WARNING_PREFIX("Thread", "Failed to set requested thread name");
	}
}
#endif // TRIVIAL_PLATFORM_WINDOWS

} // namespace

namespace trivial::thread {

Thread::~Thread() noexcept {
	if (joinable()) [[unlikely]] {
		TRIVIAL_LOG_FATAL_PREFIX("Thread", "destroyed while still joinable - call join() first");
		std::abort();
	}

	if (g_currentThread == this) {
		g_currentThread = nullptr;
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
		if (pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) != 0
		    || pthread_attr_setschedpolicy(&attr, config.schedPolicy) != 0) {
			TRIVIAL_LOG_WARNING_PREFIX("Thread", "Failed to set requested scheduling policy");
		}

		sched_param param{};
		param.sched_priority = config.schedPriority;
		if (pthread_attr_setschedparam(&attr, &param) != 0) {
			TRIVIAL_LOG_WARNING_PREFIX("Thread", "Failed to set requested scheduling priority");
		}
	}

	cpu_set_t cpuSet{};

	if (config.affinityMask != 0) {
		CPU_ZERO(&cpuSet);

		for (unsigned int bit = 0; bit < 64; ++bit) {
			if ((config.affinityMask & (std::uint64_t{1} << bit)) != 0) {
				CPU_SET((config.affinityGroup * 64) + bit, &cpuSet);
			}
		}

		if (pthread_attr_setaffinity_np(&attr, sizeof(cpuSet), &cpuSet) != 0) {
			TRIVIAL_LOG_WARNING_PREFIX("Thread", "Failed to set requested CPU affinity");
		}
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

	m_nativeHandleStorage = std::bit_cast<NativeHandleStorage>(handle);

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

	applyThreadDescription(handle, m_name);

	if (SetThreadPriority(handle, config.win32Priority) == 0) {
		TRIVIAL_LOG_WARNING_PREFIX("Thread", "Failed to set requested priority");
	}

#if TRIVIAL_PLATFORM_HAS_CPU_AFFINITY
	if (config.affinityMask != 0) {
		GROUP_AFFINITY groupAffinity{};
		groupAffinity.Mask = config.affinityMask;
		groupAffinity.Group = config.affinityGroup;

		if (SetThreadGroupAffinity(handle, &groupAffinity, nullptr) == 0) {
			TRIVIAL_LOG_WARNING_PREFIX("Thread", "Failed to set requested CPU affinity");
		}
	}
#endif // TRIVIAL_PLATFORM_HAS_CPU_AFFINITY

	m_state.store(config.createSuspended ? ThreadState::Suspended : ThreadState::Running, std::memory_order_release);

	m_nativeHandleStorage = std::bit_cast<NativeHandleStorage>(handle);

	ResumeThread(handle);
#endif // Platform-specific creation

	return ThreadCreateResult{.error = ThreadCreateError::None, .platformErrorCode = 0};
}

void Thread::adoptCurrentThread(const ThreadConfig& config) noexcept {
	TRIVIAL_ASSERT(m_state.load(std::memory_order_relaxed) == ThreadState::NotStarted);
	TRIVIAL_ASSERT(config.type == ThreadType::Main);

	m_index = g_nextThreadIndex.fetch_add(1, std::memory_order_relaxed);
	m_type = config.type;

	copyName(config.name, m_name);

	m_state.store(ThreadState::Running, std::memory_order_release);

#if TRIVIAL_PLATFORM_POSIX
	const pthread_t kHandle = pthread_self();
	m_nativeHandleStorage = std::bit_cast<NativeHandleStorage>(kHandle);

#if TRIVIAL_PLATFORM_LINUX
	if (m_name[0] != '\0') {
		pthread_setname_np(kHandle, m_name.data());
	}

	if (config.schedPolicy != SCHED_OTHER) {
		sched_param param{};
		param.sched_priority = config.schedPriority;
		if (pthread_setschedparam(kHandle, config.schedPolicy, &param) != 0) {
			TRIVIAL_LOG_WARNING_PREFIX("Thread", "Failed to set requested scheduling priority");
		}
	}

	if (config.affinityMask != 0) {
		cpu_set_t cpuSet{};
		CPU_ZERO(&cpuSet);

		for (unsigned int bit = 0; bit < 64; ++bit) {
			if ((config.affinityMask & (std::uint64_t{1} << bit)) != 0) {
				CPU_SET((config.affinityGroup * 64) + bit, &cpuSet);
			}
		}

		if (pthread_setaffinity_np(kHandle, sizeof(cpuSet), &cpuSet) != 0) {
			TRIVIAL_LOG_WARNING_PREFIX("Thread", "Failed to set requested CPU affinity");
		}
	}

#elif TRIVIAL_PLATFORM_MACOS
	if (m_name[0] != '\0') { // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
		pthread_setname_np(m_name.data());
	}

	m_qosClass = config.qosClass;
	m_qosRelativePriority = config.qosRelativePriority;
	if (pthread_set_qos_class_self_np(m_qosClass, m_qosRelativePriority) != 0) {
		TRIVIAL_LOG_WARNING_PREFIX("Thread", "Failed to set requested QoS class");
	}

#endif // Platform-specific naming/priority/affinity

#elif TRIVIAL_PLATFORM_WINDOWS
	const HANDLE kHandle = GetCurrentThread();
	m_nativeHandleStorage = std::bit_cast<NativeHandleStorage>(kHandle);

	applyThreadDescription(kHandle, m_name);

	if (SetThreadPriority(kHandle, config.win32Priority) == 0) {
		TRIVIAL_LOG_WARNING_PREFIX("Thread", "Failed to set requested priority");
	}

#if TRIVIAL_PLATFORM_HAS_CPU_AFFINITY
	if (config.affinityMask != 0) {
		GROUP_AFFINITY groupAffinity{};
		groupAffinity.Mask = config.affinityMask;
		groupAffinity.Group = config.affinityGroup;

		if (SetThreadGroupAffinity(kHandle, &groupAffinity, nullptr) == 0) {
			TRIVIAL_LOG_WARNING_PREFIX("Thread", "Failed to set requested CPU affinity");
		}
	}

#endif // TRIVIAL_PLATFORM_HAS_CPU_AFFINITY

#endif // Platform-specific handle capture/priority/affinity

	g_currentThread = this;
}

void Thread::rename(const char* name) noexcept {
	TRIVIAL_ASSERT(this == g_currentThread);

	copyName(name, m_name);

#if TRIVIAL_PLATFORM_LINUX
	if (m_name[0] != '\0') { // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
		pthread_setname_np(pthread_self(), m_name.data());
	}
#elif TRIVIAL_PLATFORM_MACOS
	if (m_name[0] != '\0') { // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
		pthread_setname_np(m_name.data());
	}
#elif TRIVIAL_PLATFORM_WINDOWS
	applyThreadDescription(GetCurrentThread(), m_name);
#endif // Platform-specific rename
}

void Thread::resume() noexcept {
	TRIVIAL_ASSERT(m_state.load(std::memory_order_relaxed) == ThreadState::Suspended);

	m_state.store(ThreadState::Running, std::memory_order_release);
	m_state.notify_one();
}

void Thread::join() noexcept {
	TRIVIAL_ASSERT(joinable());

#if TRIVIAL_PLATFORM_POSIX
	auto* const kHandle = std::bit_cast<pthread_t>(m_nativeHandleStorage);

	pthread_join(kHandle, nullptr);

	m_stackAllocator->release(m_stackAllocation);
	m_stackAllocator = nullptr;
	m_stackAllocation = ThreadStackAllocation{};
#elif TRIVIAL_PLATFORM_WINDOWS
	const auto kHandle = std::bit_cast<HANDLE>(m_nativeHandleStorage);

	WaitForSingleObject(kHandle, INFINITE);
	CloseHandle(kHandle);
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

[[nodiscard]] std::uint32_t Thread::resolveConcurrency(std::uint32_t requested) noexcept {
	if (requested != 0) {
		return requested;
	}

	const std::uint32_t kHardware = std::thread::hardware_concurrency();

	if (kHardware == 0U) {
		TRIVIAL_LOG_ERROR("Could not resolve hardware concurrency");
		return 1;
	}

	return kHardware;
}

void Thread::runEntry(Thread* self) noexcept {
	g_currentThread = self;

	if (self->m_state.load(std::memory_order_acquire) == ThreadState::Suspended) {
		self->m_state.wait(ThreadState::Suspended, std::memory_order_acquire);
	}

#if TRIVIAL_PLATFORM_MACOS
	if (pthread_set_qos_class_self_np(self->m_qosClass, self->m_qosRelativePriority) != 0) {
		TRIVIAL_LOG_WARNING_PREFIX("Thread", "Failed to set requested QoS class");
	}

	if (self->m_name[0] != '\0') { // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
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

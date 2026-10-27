#define TRIVIAL_THREAD_IMPLEMENTATION
#include <trivial/core/thread/thread.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cstdint>
#include <cstring>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX

#include <windows.h> // IWYU pragma: keep

#include <trivial/core/assert.h>
#include <trivial/core/log.h>
#include <trivial/core/platform.h>
#include <trivial/core/thread/thread_config.h>

namespace {

// Non-atomic would be safe but cost is much less than the syscalls for thread
// creation and makes it safer for future planned usage changes
std::atomic<std::uint32_t> g_nextThreadId{0}; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

void copyName(const char* name, std::array<char, TRIVIAL_THREAD_MAX_NAME_LENGTH>& outName) noexcept {
	if (name == nullptr) {
		outName[0] = '\0';
		return;
	}

	const std::size_t kLength = std::min(std::strlen(name), outName.size() - 1);

	std::memcpy(outName.data(), name, kLength);
	outName[kLength] = '\0';
}

void applyThreadDescription(HANDLE handle, const std::array<char, TRIVIAL_THREAD_MAX_NAME_LENGTH>& name) noexcept {
	if (name[0] == '\0') {
		return;
	}

	std::array<wchar_t, TRIVIAL_THREAD_MAX_NAME_LENGTH> wideName{};

	std::size_t i = 0;
	for (; i < name.size() - 1 && name[i] != '\0'; ++i) {
		wideName[i] = static_cast<wchar_t>(static_cast<unsigned char>(name[i]));
	}
	wideName[i] = L'\0';

	if (FAILED(SetThreadDescription(handle, wideName.data()))) {
		TRIVIAL_LOG_WARNING_PREFIX("Thread", "Failed to set requested thread name");
	}
}

} // namespace

namespace trivial::thread {

[[nodiscard]] ThreadCreateResult Thread::create(const ThreadConfig& config,
                                                ThreadStartRoutine startRoutine,
                                                void* arg) noexcept {
	TRIVIAL_ASSERT(m_state.load(std::memory_order_relaxed) == ThreadState::NotStarted);
	TRIVIAL_ASSERT(startRoutine != nullptr);

	m_id = g_nextThreadId.fetch_add(1, std::memory_order_relaxed);
	m_type = config.type;
	m_startRoutine = startRoutine;
	m_arg = arg;

	copyName(config.name, m_name);

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

	return ThreadCreateResult{.error = ThreadCreateError::None, .platformErrorCode = 0};
}

void Thread::adoptCurrentThread(const ThreadConfig& config) noexcept {
	TRIVIAL_ASSERT(m_state.load(std::memory_order_relaxed) == ThreadState::NotStarted);
	TRIVIAL_ASSERT(config.type == ThreadType::Main);

	m_id = g_nextThreadId.fetch_add(1, std::memory_order_relaxed);
	m_type = config.type;

	copyName(config.name, m_name);

	m_state.store(ThreadState::Running, std::memory_order_release);

	const HANDLE kHandle = GetCurrentThread(); // NOLINT(misc-misplaced-const)
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

	detail::g_currentThread = this;
}

void Thread::rename(const char* name) noexcept {
	TRIVIAL_ASSERT(this == detail::g_currentThread);

	copyName(name, m_name);

	applyThreadDescription(GetCurrentThread(), m_name);
}

void Thread::join() noexcept {
	TRIVIAL_ASSERT(joinable());

	const HANDLE kHandle = std::bit_cast<HANDLE>(m_nativeHandleStorage); // NOLINT(misc-misplaced-const)

	WaitForSingleObject(kHandle, INFINITE);
	CloseHandle(kHandle);

	m_state.store(ThreadState::Joined, std::memory_order_release);
}

void Thread::yield() noexcept {
	if constexpr (TRIVIAL_THREAD_WINDOWS_YIELD_USE_SLEEP_ZERO) {
		Sleep(0);
	} else {
		SwitchToThread();
	}
}

void Thread::runEntry(Thread* self) noexcept {
	detail::g_currentThread = self;

	if (self->m_state.load(std::memory_order_acquire) == ThreadState::Suspended) {
		self->m_state.wait(ThreadState::Suspended, std::memory_order_acquire);
	}

	self->m_startRoutine(self->m_arg);
}

unsigned long __stdcall Thread::win32ThreadEntry(void* arg) noexcept {
	runEntry(static_cast<Thread*>(arg));
	return 0;
}

} // namespace trivial::thread

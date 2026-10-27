#define TRIVIAL_THREAD_IMPLEMENTATION
#include <trivial/core/thread/thread.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <pthread.h>
#include <sched.h>

#include <trivial/core/assert.h>
#include <trivial/core/log.h>
#include <trivial/core/thread/thread_stack_allocator.h>

#define TRIVIAL_THREAD_AFFINITY_MASK_BITS 64U

static_assert(sizeof(pthread_t) == sizeof(void*), "pthread_t is no longer NativeHandleStorage-sized");

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

		for (unsigned int bit = 0; bit < TRIVIAL_THREAD_AFFINITY_MASK_BITS; ++bit) {
			if ((config.affinityMask & (std::uint64_t{1} << bit)) != 0) {
				CPU_SET((config.affinityGroup * TRIVIAL_THREAD_AFFINITY_MASK_BITS) + bit, &cpuSet);
			}
		}

		if (pthread_attr_setaffinity_np(&attr, sizeof(cpuSet), &cpuSet) != 0) {
			TRIVIAL_LOG_WARNING_PREFIX("Thread", "Failed to set requested CPU affinity");
		}
	}

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

	return ThreadCreateResult{.error = ThreadCreateError::None, .platformErrorCode = 0};
}

void Thread::adoptCurrentThread(const ThreadConfig& config) noexcept {
	TRIVIAL_ASSERT(m_state.load(std::memory_order_relaxed) == ThreadState::NotStarted);
	TRIVIAL_ASSERT(config.type == ThreadType::Main);

	m_id = g_nextThreadId.fetch_add(1, std::memory_order_relaxed);
	m_type = config.type;

	copyName(config.name, m_name);

	m_state.store(ThreadState::Running, std::memory_order_release);

	const pthread_t kHandle = pthread_self();
	m_nativeHandleStorage = std::bit_cast<NativeHandleStorage>(kHandle);

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

		for (unsigned int bit = 0; bit < TRIVIAL_THREAD_AFFINITY_MASK_BITS; ++bit) {
			if ((config.affinityMask & (std::uint64_t{1} << bit)) != 0) {
				CPU_SET((config.affinityGroup * TRIVIAL_THREAD_AFFINITY_MASK_BITS) + bit, &cpuSet);
			}
		}

		if (pthread_setaffinity_np(kHandle, sizeof(cpuSet), &cpuSet) != 0) {
			TRIVIAL_LOG_WARNING_PREFIX("Thread", "Failed to set requested CPU affinity");
		}
	}

	detail::g_currentThread = this;
}

void Thread::rename(const char* name) noexcept {
	TRIVIAL_ASSERT(this == detail::g_currentThread);

	copyName(name, m_name);

	if (m_name[0] != '\0') {
		pthread_setname_np(pthread_self(), m_name.data());
	}
}

void Thread::join() noexcept {
	TRIVIAL_ASSERT(joinable());

	const pthread_t kHandle = std::bit_cast<pthread_t>(m_nativeHandleStorage);

	pthread_join(kHandle, nullptr);

	m_stackAllocator->release(m_stackAllocation);
	m_stackAllocator = nullptr;
	m_stackAllocation = ThreadStackAllocation{};

	m_state.store(ThreadState::Joined, std::memory_order_release);
}

void Thread::yield() noexcept {
	sched_yield();
}

void Thread::runEntry(Thread* self) noexcept {
	detail::g_currentThread = self;

	if (self->m_state.load(std::memory_order_acquire) == ThreadState::Suspended) {
		self->m_state.wait(ThreadState::Suspended, std::memory_order_acquire);
	}

	if (self->m_name[0] != '\0') {
		pthread_setname_np(pthread_self(), self->m_name.data());
	}

	self->m_startRoutine(self->m_arg);
}

void* Thread::posixThreadEntry(void* arg) noexcept {
	runEntry(static_cast<Thread*>(arg));
	return nullptr;
}

} // namespace trivial::thread

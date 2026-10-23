#define TRIVIAL_THREAD_IMPLEMENTATION
#include <trivial/core/thread/thread.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <mach/kern_return.h>
#include <mach/mach_init.h>
#include <mach/mach_port.h>
#include <mach/mach_traps.h>
#include <mach/message.h>
#include <mach/port.h>
#include <mach/thread_act.h>
#include <mach/thread_info.h>
#include <mach/thread_switch.h>
#include <pthread.h>
#include <pthread/qos.h>

#include <trivial/core/assert.h>
#include <trivial/core/log.h>
#include <trivial/core/thread/thread_config.h>
#include <trivial/core/thread/thread_stack_allocator.h>

static_assert(sizeof(pthread_t) == sizeof(void*), "pthread_t is no longer NativeHandleStorage-sized");
static_assert(sizeof(mach_port_t) == sizeof(std::uint32_t), "mach_port_t no longer fits m_machThreadPort");

namespace {

// Non-atomic would be safe but cost is much less than the syscalls for thread
// creation and makes it safer for future planned usage changes
std::atomic<std::uint32_t> g_nextThreadIndex{0}; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

void copyName(const char* name, std::array<char, TRIVIAL_THREAD_MAX_NAME_LENGTH>& outName) noexcept {
	if (name == nullptr) {
		outName[0] = '\0';
		return;
	}

	const std::size_t kLength = std::min(std::strlen(name), outName.size() - 1);

	std::memcpy(outName.data(), name, kLength);
	outName[kLength] = '\0';
}

void waitForKernelThreadExit(mach_port_t port) noexcept {
	for (;;) {
		thread_basic_info_data_t info{};
		mach_msg_type_number_t count = THREAD_BASIC_INFO_COUNT;
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		if (thread_info(port, THREAD_BASIC_INFO, reinterpret_cast<thread_info_t>(&info), &count) != KERN_SUCCESS) {
			return;
		}

		trivial::thread::Thread::yield();
	}
}

} // namespace

namespace trivial::thread {

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

	m_qosClass = config.qosClass;
	m_qosRelativePriority = config.qosRelativePriority;

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

	m_index = g_nextThreadIndex.fetch_add(1, std::memory_order_relaxed);
	m_type = config.type;

	copyName(config.name, m_name);

	m_state.store(ThreadState::Running, std::memory_order_release);

	const pthread_t kHandle = pthread_self();
	m_nativeHandleStorage = std::bit_cast<NativeHandleStorage>(kHandle);

	if (m_name[0] != '\0') {
		pthread_setname_np(m_name.data());
	}

	m_qosClass = config.qosClass;
	m_qosRelativePriority = config.qosRelativePriority;
	if (pthread_set_qos_class_self_np(m_qosClass, m_qosRelativePriority) != 0) {
		TRIVIAL_LOG_WARNING_PREFIX("Thread", "Failed to set requested QoS class");
	}

	detail::g_currentThread = this;
}

void Thread::rename(const char* name) noexcept {
	TRIVIAL_ASSERT(this == detail::g_currentThread);

	copyName(name, m_name);

	if (m_name[0] != '\0') {
		pthread_setname_np(m_name.data());
	}
}

void Thread::join() noexcept {
	TRIVIAL_ASSERT(joinable());

	const pthread_t kHandle = std::bit_cast<pthread_t>(m_nativeHandleStorage);

	pthread_join(kHandle, nullptr);

	waitForKernelThreadExit(m_machThreadPort);
	(void)mach_port_deallocate(mach_task_self(), m_machThreadPort);
	m_machThreadPort = 0;

	m_stackAllocator->release(m_stackAllocation);
	m_stackAllocator = nullptr;
	m_stackAllocation = ThreadStackAllocation{};

	m_state.store(ThreadState::Joined, std::memory_order_release);
}

void Thread::yield() noexcept {
	thread_switch(MACH_PORT_NULL, SWITCH_OPTION_DEPRESS, TRIVIAL_THREAD_MACOS_YIELD_DEPRESS_TIMEOUT_MS);
}

void Thread::runEntry(Thread* self) noexcept {
	detail::g_currentThread = self;
	self->m_machThreadPort = mach_thread_self();

	if (self->m_state.load(std::memory_order_acquire) == ThreadState::Suspended) {
		self->m_state.wait(ThreadState::Suspended, std::memory_order_acquire);
	}

	if (pthread_set_qos_class_self_np(self->m_qosClass, self->m_qosRelativePriority) != 0) {
		TRIVIAL_LOG_WARNING_PREFIX("Thread", "Failed to set requested QoS class");
	}

	if (self->m_name[0] != '\0') {
		pthread_setname_np(self->m_name.data());
	}

	self->m_startRoutine(self->m_arg);
}

void* Thread::posixThreadEntry(void* arg) noexcept {
	runEntry(static_cast<Thread*>(arg));
	return nullptr;
}

} // namespace trivial::thread

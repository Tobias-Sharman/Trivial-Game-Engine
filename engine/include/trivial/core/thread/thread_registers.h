#ifndef TRIVIAL_CORE_THREAD_THREAD_REGISTERS_H
#define TRIVIAL_CORE_THREAD_THREAD_REGISTERS_H

#include <trivial/core/compiler.h>
#include <trivial/core/platform.h>

#if TRIVIAL_PLATFORM_WINDOWS
#include <bit>
#include <intrin.h>
#endif // TRIVIAL_PLATFORM_WINDOWS

namespace trivial::thread {

[[nodiscard]] TRIVIAL_FORCE_INLINE void* osThreadPointer() noexcept {
#if TRIVIAL_PLATFORM_WINDOWS && TRIVIAL_ARCH_X86_64
	return std::bit_cast<void*>(__readgsqword(0x30));
#elif TRIVIAL_PLATFORM_WINDOWS && TRIVIAL_ARCH_ARM64
	return std::bit_cast<void*>(__getReg(18));
#elif TRIVIAL_PLATFORM_MACOS && TRIVIAL_ARCH_ARM64
	void* pointer = nullptr;
	__asm__("mrs %0, tpidrro_el0\n\tbic %0, %0, #7" : "=r"(pointer)); // NOLINT(portability-no-assembler)
	return pointer;
#elif TRIVIAL_PLATFORM_MACOS
	void* pointer = nullptr;
	__asm__("movq %%gs:0, %0" : "=r"(pointer)); // NOLINT(portability-no-assembler)
	return pointer;
#else
	return __builtin_thread_pointer();
#endif // Thread pointer
}

} // namespace trivial::thread

#endif // TRIVIAL_CORE_THREAD_THREAD_REGISTERS_H

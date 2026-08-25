#ifndef TRIVIAL_CORE_THREAD_THREAD_CONFIG_H
#define TRIVIAL_CORE_THREAD_THREAD_CONFIG_H

#include <cstddef>

#include <trivial/core/config.h>
#include <trivial/core/platform.h>

#ifndef TRIVIAL_THREAD_DEFAULT_STACK_SIZE_BYTES
#define TRIVIAL_THREAD_DEFAULT_STACK_SIZE_BYTES (std::size_t{2} << 20) // 2 MiB
#endif

#ifndef TRIVIAL_THREAD_WINDOWS_YIELD_USE_SLEEP_ZERO
#define TRIVIAL_THREAD_WINDOWS_YIELD_USE_SLEEP_ZERO 1
#endif

#if (TRIVIAL_THREAD_WINDOWS_YIELD_USE_SLEEP_ZERO != 0) && (TRIVIAL_THREAD_WINDOWS_YIELD_USE_SLEEP_ZERO != 1)
#error "TRIVIAL_THREAD_WINDOWS_YIELD_USE_SLEEP_ZERO must be 0 or 1"
#endif

// 1ms, not sched_yield()'s ~10ms depression on Darwin - see:
// https://bugs.webkit.org/show_bug.cgi?id=215248
// https://github.com/chromium/chromium/blob/main/base/threading/platform_thread_apple.mm
#ifndef TRIVIAL_THREAD_MACOS_YIELD_DEPRESS_TIMEOUT_MS
#define TRIVIAL_THREAD_MACOS_YIELD_DEPRESS_TIMEOUT_MS 1
#endif

#endif // TRIVIAL_CORE_THREAD_THREAD_CONFIG_H

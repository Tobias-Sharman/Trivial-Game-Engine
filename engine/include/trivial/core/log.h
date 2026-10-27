#ifndef TRIVIAL_CORE_LOG_H
#define TRIVIAL_CORE_LOG_H

#include <cstdint>

#include <trivial/core/config.h>

#if TRIVIAL_ENABLE_LOGGING
#include <cstddef>

#include <trivial/core/compiler.h>
#include <trivial/core/log_config.h>
#endif // TRIVIAL_ENABLE_LOGGING

namespace trivial::core {

enum class LogLevel : std::uint8_t {
	Debug,
	Info,
	Warning,
	Error,
	Fatal,
};

#if TRIVIAL_ENABLE_LOGGING

void logMessage(LogLevel level, const char* message) noexcept;
void logMessageWithPrefix(LogLevel level, const char* prefix, const char* message) noexcept;
TRIVIAL_COLD void logOomFailure(const char* prefix,
                                const char* context,
                                std::size_t requestedSize,
                                int osErrorCode) noexcept;

TRIVIAL_COLD void logCpuInfo() noexcept;
TRIVIAL_COLD void logCpuTopology() noexcept;
TRIVIAL_COLD void logMemoryInfo() noexcept;
TRIVIAL_COLD void logTimeInfo() noexcept;
TRIVIAL_COLD void logSystemInfo() noexcept;

#endif // TRIVIAL_ENABLE_LOGGING

} // namespace trivial::core

#if TRIVIAL_ENABLE_LOGGING

#define TRIVIAL_LOG_DEBUG(message) ::trivial::core::logMessage(::trivial::core::LogLevel::Debug, (message))
#define TRIVIAL_LOG_INFO(message) ::trivial::core::logMessage(::trivial::core::LogLevel::Info, (message))
#define TRIVIAL_LOG_WARNING(message) ::trivial::core::logMessage(::trivial::core::LogLevel::Warning, (message))
#define TRIVIAL_LOG_ERROR(message) ::trivial::core::logMessage(::trivial::core::LogLevel::Error, (message))
#define TRIVIAL_LOG_FATAL(message) ::trivial::core::logMessage(::trivial::core::LogLevel::Fatal, (message))

#define TRIVIAL_LOG_DEBUG_PREFIX(prefix, message)                                                                      \
	::trivial::core::logMessageWithPrefix(::trivial::core::LogLevel::Debug, (prefix), (message))
#define TRIVIAL_LOG_INFO_PREFIX(prefix, message)                                                                       \
	::trivial::core::logMessageWithPrefix(::trivial::core::LogLevel::Info, (prefix), (message))
#define TRIVIAL_LOG_WARNING_PREFIX(prefix, message)                                                                    \
	::trivial::core::logMessageWithPrefix(::trivial::core::LogLevel::Warning, (prefix), (message))
#define TRIVIAL_LOG_ERROR_PREFIX(prefix, message)                                                                      \
	::trivial::core::logMessageWithPrefix(::trivial::core::LogLevel::Error, (prefix), (message))
#define TRIVIAL_LOG_FATAL_PREFIX(prefix, message)                                                                      \
	::trivial::core::logMessageWithPrefix(::trivial::core::LogLevel::Fatal, (prefix), (message))

#define TRIVIAL_LOG_OOM_FAILURE(prefix, context, requestedSize, osErrorCode)                                           \
	::trivial::core::logOomFailure((prefix), (context), (requestedSize), (osErrorCode))

#define TRIVIAL_LOG_CPU_INFO() ::trivial::core::logCpuInfo()
#define TRIVIAL_LOG_CPU_TOPOLOGY() ::trivial::core::logCpuTopology()
#define TRIVIAL_LOG_MEMORY_INFO() ::trivial::core::logMemoryInfo()
#define TRIVIAL_LOG_TIME_INFO() ::trivial::core::logTimeInfo()
#define TRIVIAL_LOG_SYSTEM_INFO() ::trivial::core::logSystemInfo()

#if TRIVIAL_LOG_SYSTEM_INFO_AT_STARTUP
#define TRIVIAL_LOG_STARTUP_SYSTEM_INFO()                                                                              \
	do {                                                                                                               \
		if constexpr (TRIVIAL_LOG_CPU_INFO_AT_STARTUP) {                                                               \
			::trivial::core::logCpuInfo();                                                                             \
		}                                                                                                              \
		if constexpr (TRIVIAL_LOG_CPU_TOPOLOGY_AT_STARTUP) {                                                           \
			::trivial::core::logCpuTopology();                                                                         \
		}                                                                                                              \
		if constexpr (TRIVIAL_LOG_MEMORY_INFO_AT_STARTUP) {                                                            \
			::trivial::core::logMemoryInfo();                                                                          \
		}                                                                                                              \
		if constexpr (TRIVIAL_LOG_TIME_INFO_AT_STARTUP) {                                                              \
			::trivial::core::logTimeInfo();                                                                            \
		}                                                                                                              \
	} while (false)
#else
#define TRIVIAL_LOG_STARTUP_SYSTEM_INFO() ((void)0)
#endif // TRIVIAL_LOG_SYSTEM_INFO_AT_STARTUP

#else

#define TRIVIAL_LOG_DEBUG(message) ((void)0)
#define TRIVIAL_LOG_INFO(message) ((void)0)
#define TRIVIAL_LOG_WARNING(message) ((void)0)
#define TRIVIAL_LOG_ERROR(message) ((void)0)
#define TRIVIAL_LOG_FATAL(message) ((void)0)

#define TRIVIAL_LOG_DEBUG_PREFIX(prefix, message) ((void)0)
#define TRIVIAL_LOG_INFO_PREFIX(prefix, message) ((void)0)
#define TRIVIAL_LOG_WARNING_PREFIX(prefix, message) ((void)0)
#define TRIVIAL_LOG_ERROR_PREFIX(prefix, message) ((void)0)
#define TRIVIAL_LOG_FATAL_PREFIX(prefix, message) ((void)0)

#define TRIVIAL_LOG_OOM_FAILURE(prefix, context, requestedSize, osErrorCode) ((void)0)

#define TRIVIAL_LOG_CPU_INFO() ((void)0)
#define TRIVIAL_LOG_CPU_TOPOLOGY() ((void)0)
#define TRIVIAL_LOG_MEMORY_INFO() ((void)0)
#define TRIVIAL_LOG_TIME_INFO() ((void)0)
#define TRIVIAL_LOG_SYSTEM_INFO() ((void)0)

#define TRIVIAL_LOG_STARTUP_SYSTEM_INFO() ((void)0)

#endif // TRIVIAL_ENABLE_LOGGING

#endif // TRIVIAL_CORE_LOG_H

#ifndef TRIVIAL_CORE_LOG_H
#define TRIVIAL_CORE_LOG_H

#include <cstdint>

#include <trivial/core/config.h>

namespace trivial::internal::core {

enum class LogLevel : uint8_t {
	Debug,
	Info,
	Warning,
	Error,
	Fatal
};

#if TRIVIAL_ENABLE_LOGGING

void logMessage(LogLevel level, const char* message);
void logMessageWithPrefix(LogLevel level, const char* prefix, const char* message);

#define TRIVIAL_LOG_DEBUG(message)                                                                                     \
	::trivial::internal::core::logMessage(::trivial::internal::core::LogLevel::Debug, message)
#define TRIVIAL_LOG_INFO(message)                                                                                      \
	::trivial::internal::core::logMessage(::trivial::internal::core::LogLevel::Info, message)
#define TRIVIAL_LOG_WARNING(message)                                                                                   \
	::trivial::internal::core::logMessage(::trivial::internal::core::LogLevel::Warning, message)
#define TRIVIAL_LOG_ERROR(message)                                                                                     \
	::trivial::internal::core::logMessage(::trivial::internal::core::LogLevel::Error, message)
#define TRIVIAL_LOG_FATAL(message)                                                                                     \
	::trivial::internal::core::logMessage(::trivial::internal::core::LogLevel::Fatal, message)

#define TRIVIAL_LOG_DEBUG_PREFIX(prefix, message)                                                                      \
	::trivial::internal::core::logMessageWithPrefix(::trivial::internal::core::LogLevel::Debug, prefix, message)
#define TRIVIAL_LOG_INFO_PREFIX(prefix, message)                                                                       \
	::trivial::internal::core::logMessageWithPrefix(::trivial::internal::core::LogLevel::Info, prefix, message)
#define TRIVIAL_LOG_WARNING_PREFIX(prefix, message)                                                                    \
	::trivial::internal::core::logMessageWithPrefix(::trivial::internal::core::LogLevel::Warning, prefix, message)
#define TRIVIAL_LOG_ERROR_PREFIX(prefix, message)                                                                      \
	::trivial::internal::core::logMessageWithPrefix(::trivial::internal::core::LogLevel::Error, prefix, message)
#define TRIVIAL_LOG_FATAL_PREFIX(prefix, message)                                                                      \
	::trivial::internal::core::logMessageWithPrefix(::trivial::internal::core::LogLevel::Fatal, prefix, message)

#else

#define TRIVIAL_LOG_DEBUG(message) ((void)0)
#define TRIVIAL_LOG_INFO(message) ((void)0)
#define TRIVIAL_LOG_WARNING(message) ((void)0)
#define TRIVIAL_LOG_ERROR(message) ((void)0)
#define TRIVIAL_LOG_FATAL(message) ((void)0)

#define TRIVIAL_LOG_DEBUG_PREFIX(message) ((void)0)
#define TRIVIAL_LOG_INFO_PREFIX(message) ((void)0)
#define TRIVIAL_LOG_WARNING_PREFIX(message) ((void)0)
#define TRIVIAL_LOG_ERROR_PREFIX(message) ((void)0)
#define TRIVIAL_LOG_FATAL_PREFIX(message) ((void)0)

#endif // TRIVIAL_ENABLE_LOGGING

} // namespace trivial::internal::core

#endif // TRIVIAL_CORE_LOG_H

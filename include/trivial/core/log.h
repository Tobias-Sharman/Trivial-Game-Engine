#ifndef TRIVIAL_CORE_LOG_H
#define TRIVIAL_CORE_LOG_H

#include <cstddef>
#include <cstdint>

#include <trivial/core/config.h>

namespace trivial::core {

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
void logOomFailure(const char* prefix, const char* context, std::size_t requestedSize, int osErrorCode);

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TRIVIAL_LOG_DEBUG(message) ::trivial::core::logMessage(::trivial::core::LogLevel::Debug, message)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TRIVIAL_LOG_INFO(message) ::trivial::core::logMessage(::trivial::core::LogLevel::Info, message)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TRIVIAL_LOG_WARNING(message) ::trivial::core::logMessage(::trivial::core::LogLevel::Warning, message)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TRIVIAL_LOG_ERROR(message) ::trivial::core::logMessage(::trivial::core::LogLevel::Error, message)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TRIVIAL_LOG_FATAL(message) ::trivial::core::logMessage(::trivial::core::LogLevel::Fatal, message)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TRIVIAL_LOG_DEBUG_PREFIX(prefix, message)                                                                      \
	::trivial::core::logMessageWithPrefix(::trivial::core::LogLevel::Debug, prefix, message)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TRIVIAL_LOG_INFO_PREFIX(prefix, message)                                                                       \
	::trivial::core::logMessageWithPrefix(::trivial::core::LogLevel::Info, prefix, message)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TRIVIAL_LOG_WARNING_PREFIX(prefix, message)                                                                    \
	::trivial::core::logMessageWithPrefix(::trivial::core::LogLevel::Warning, prefix, message)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TRIVIAL_LOG_ERROR_PREFIX(prefix, message)                                                                      \
	::trivial::core::logMessageWithPrefix(::trivial::core::LogLevel::Error, prefix, message)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TRIVIAL_LOG_FATAL_PREFIX(prefix, message)                                                                      \
	::trivial::core::logMessageWithPrefix(::trivial::core::LogLevel::Fatal, prefix, message)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TRIVIAL_LOG_OOM_FAILURE(prefix, context, requestedSize, osErrorCode)                                           \
	::trivial::core::logOomFailure(prefix, context, requestedSize, osErrorCode)

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

#define TRIVIAL_LOG_OOM_FAILURE(prefix, context, requestedSize, osErrorCode) ((void)0)

#endif // TRIVIAL_ENABLE_LOGGING

} // namespace trivial::core

#endif // TRIVIAL_CORE_LOG_H

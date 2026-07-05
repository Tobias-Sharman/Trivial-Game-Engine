#include <trivial/core/log.h>

#if TRIVIAL_ENABLE_LOGGING

#include <cstdio>

namespace trivial::internal::core {

namespace {

const char* logLevelName(LogLevel logLevel) {
	switch (logLevel) {
		case LogLevel::Debug:
			return "debug";
		case LogLevel::Info:
			return "info";
		case LogLevel::Warning:
			return "warning";
		case LogLevel::Error:
			return "error";
		case LogLevel::Fatal:
			return "Fatal";
		default:
			return "unkown";
	}
}

} // namespace

void logMessage(LogLevel level, const char* message) {
	(void)std::fputs("Trivial ", stderr);
	(void)std::fputs(logLevelName(level), stderr);
	(void)std::fputs(": ", stderr);
	(void)std::fputs(message, stderr);
	(void)std::fputc('\n', stderr);
}

void logMessageWithPrefix(LogLevel level, const char* prefix, const char* message) {
	(void)std::fputs("Trivial ", stderr);
	(void)std::fputs(logLevelName(level), stderr);
	(void)std::fputs(": ", stderr);
	(void)std::fputs(prefix, stderr);
	(void)std::fputs(message, stderr);
	(void)std::fputc('\n', stderr);
}

} // namespace trivial::internal::core

#endif // TRIVIAL_ENABLE_LOGGING

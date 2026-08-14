#include <trivial/core/log.h>

#if TRIVIAL_ENABLE_LOGGING

#include <cstdio>

namespace trivial::core {

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
			return "fatal";
		default:
			return "unknown";
	}
}

} // namespace

// TODO: proper logging system safe under multithreading

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
	(void)std::fputs(" [", stderr);
	(void)std::fputs(prefix, stderr);
	(void)std::fputs("]: ", stderr);
	(void)std::fputs(message, stderr);
	(void)std::fputc('\n', stderr);
}

void logOomFailure(const char* prefix, const char* context, std::size_t requestedSize, int osErrorCode) {
	(void)std::fprintf(stderr,
	                   "Trivial %s [%s]: %s (requested %zu bytes, os error %d)\n",
	                   logLevelName(LogLevel::Fatal),
	                   prefix,
	                   context,
	                   requestedSize,
	                   osErrorCode);
}

} // namespace trivial::core

#endif // TRIVIAL_ENABLE_LOGGING

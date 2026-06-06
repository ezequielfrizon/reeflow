#include "core/logging/logger.h"

#include <stdio.h>

namespace reeflow::core::logging {
namespace {

constexpr const char* kUnknownOrigin = "unknown";
constexpr const char* kEmptyMessage = "";
constexpr size_t kLogLineBufferLength = 160;

int levelRank(LogLevel level) {
  switch (level) {
    case LogLevel::kDebug:
      return 0;
    case LogLevel::kInfo:
      return 1;
    case LogLevel::kWarning:
      return 2;
    case LogLevel::kError:
      return 3;
  }

  return 3;
}

}  // namespace

Logger::Logger(platform::LogSink& sink) : sink_(sink) {}

void Logger::setMinimumLevel(LogLevel level) {
  minimumLevel_ = level;
}

LogLevel Logger::minimumLevel() const {
  return minimumLevel_;
}

void Logger::log(LogLevel level, const char* origin, const char* message) {
  if (!shouldLog(level)) {
    return;
  }

  const char* safeOrigin = origin == nullptr ? kUnknownOrigin : origin;
  const char* safeMessage = message == nullptr ? kEmptyMessage : message;
  char line[kLogLineBufferLength] = {};
  snprintf(line, sizeof(line), "[%s] %s: %s\n", logLevelName(level),
           safeOrigin, safeMessage);
  sink_.write(line);
}

void Logger::debug(const char* origin, const char* message) {
  log(LogLevel::kDebug, origin, message);
}

void Logger::info(const char* origin, const char* message) {
  log(LogLevel::kInfo, origin, message);
}

void Logger::warning(const char* origin, const char* message) {
  log(LogLevel::kWarning, origin, message);
}

void Logger::error(const char* origin, const char* message) {
  log(LogLevel::kError, origin, message);
}

bool Logger::shouldLog(LogLevel level) const {
  return levelRank(level) >= levelRank(minimumLevel_);
}

const char* logLevelName(LogLevel level) {
  switch (level) {
    case LogLevel::kDebug:
      return "DEBUG";
    case LogLevel::kInfo:
      return "INFO";
    case LogLevel::kWarning:
      return "WARNING";
    case LogLevel::kError:
      return "ERROR";
  }

  return "ERROR";
}

}  // namespace reeflow::core::logging

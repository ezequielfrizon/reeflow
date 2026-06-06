#pragma once

#include "core/platform/log_sink.h"

namespace reeflow::core::logging {

enum class LogLevel {
  kDebug,
  kInfo,
  kWarning,
  kError,
};

class Logger {
 public:
  explicit Logger(platform::LogSink& sink);

  void setMinimumLevel(LogLevel level);
  LogLevel minimumLevel() const;

  void log(LogLevel level, const char* origin, const char* message);
  void debug(const char* origin, const char* message);
  void info(const char* origin, const char* message);
  void warning(const char* origin, const char* message);
  void error(const char* origin, const char* message);

 private:
  bool shouldLog(LogLevel level) const;

  platform::LogSink& sink_;
  LogLevel minimumLevel_ = LogLevel::kDebug;
};

const char* logLevelName(LogLevel level);

}  // namespace reeflow::core::logging

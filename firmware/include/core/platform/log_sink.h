#pragma once

namespace reeflow::core::platform {

class LogSink {
 public:
  virtual ~LogSink() = default;

  virtual void write(const char* message) = 0;
};

}  // namespace reeflow::core::platform

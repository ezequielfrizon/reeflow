#include "core/platform/arduino_core_platform.h"

#include <Arduino.h>

namespace reeflow::core::platform {
namespace {

class ArduinoTimeSource final : public TimeSource {
 public:
  uint32_t uptimeMillis() const override {
    return static_cast<uint32_t>(millis());
  }
};

class SerialLogSink final : public LogSink {
 public:
  void write(const char* message) override {
    if (message != nullptr) {
      Serial.print(message);
    }
  }
};

class NeutralWatchdogBackend final : public WatchdogBackend {
 public:
  void configure(uint32_t timeoutMillis) override {
    configuredTimeoutMillis_ = timeoutMillis;
  }

  void feed() override {
    fed_ = true;
  }

 private:
  uint32_t configuredTimeoutMillis_ = 0;
  bool fed_ = false;
};

ArduinoTimeSource timeSource;
SerialLogSink logSink;
NeutralWatchdogBackend watchdogBackend;
CorePlatform platform(timeSource, logSink, watchdogBackend);

}  // namespace

CorePlatform& arduinoCorePlatform() {
  return platform;
}

}  // namespace reeflow::core::platform

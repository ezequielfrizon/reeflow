#include "core/platform/core_platform.h"

namespace reeflow::core::platform {

CorePlatform::CorePlatform(TimeSource& timeSource, LogSink& logSink,
                           WatchdogBackend& watchdogBackend)
    : timeSource_(timeSource),
      logSink_(logSink),
      watchdogBackend_(watchdogBackend) {}

TimeSource& CorePlatform::timeSource() {
  return timeSource_;
}

LogSink& CorePlatform::logSink() {
  return logSink_;
}

WatchdogBackend& CorePlatform::watchdogBackend() {
  return watchdogBackend_;
}

}  // namespace reeflow::core::platform

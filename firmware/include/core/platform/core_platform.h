#pragma once

#include "core/platform/log_sink.h"
#include "core/platform/time_source.h"
#include "core/platform/watchdog_backend.h"

namespace reeflow::core::platform {

class CorePlatform {
 public:
  CorePlatform(TimeSource& timeSource, LogSink& logSink,
               WatchdogBackend& watchdogBackend);

  TimeSource& timeSource();
  LogSink& logSink();
  WatchdogBackend& watchdogBackend();

 private:
  TimeSource& timeSource_;
  LogSink& logSink_;
  WatchdogBackend& watchdogBackend_;
};

}  // namespace reeflow::core::platform

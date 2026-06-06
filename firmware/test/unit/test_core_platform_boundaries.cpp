#include <assert.h>
#include <stdint.h>
#include <string>

#include "core/platform/core_platform.h"
#include "fakes/fake_log_sink.h"
#include "fakes/fake_time_source.h"
#include "fakes/fake_watchdog_backend.h"

namespace {

using reeflow::core::platform::CorePlatform;
using reeflow::test::fakes::FakeLogSink;
using reeflow::test::fakes::FakeTimeSource;
using reeflow::test::fakes::FakeWatchdogBackend;

void testCorePlatformUsesInjectedTimeSource() {
  FakeTimeSource timeSource;
  FakeLogSink logSink;
  FakeWatchdogBackend watchdogBackend;
  CorePlatform platform(timeSource, logSink, watchdogBackend);

  assert(&platform.timeSource() == &timeSource);
  assert(platform.timeSource().uptimeMillis() == 0);

  timeSource.setUptimeMillis(1200);
  assert(platform.timeSource().uptimeMillis() == 1200);

  timeSource.advanceMillis(300);
  assert(platform.timeSource().uptimeMillis() == 1500);
}

void testCorePlatformUsesInjectedLogSink() {
  FakeTimeSource timeSource;
  FakeLogSink logSink;
  FakeWatchdogBackend watchdogBackend;
  CorePlatform platform(timeSource, logSink, watchdogBackend);

  assert(&platform.logSink() == &logSink);

  platform.logSink().write("core boundary ready");

  assert(logSink.messages().size() == 1);
  assert(logSink.messages()[0] == std::string("core boundary ready"));
}

void testCorePlatformUsesInjectedWatchdogBackend() {
  FakeTimeSource timeSource;
  FakeLogSink logSink;
  FakeWatchdogBackend watchdogBackend;
  CorePlatform platform(timeSource, logSink, watchdogBackend);

  assert(&platform.watchdogBackend() == &watchdogBackend);

  platform.watchdogBackend().configure(5000);
  platform.watchdogBackend().feed();
  platform.watchdogBackend().feed();

  assert(watchdogBackend.configureCalls() == 1);
  assert(watchdogBackend.configuredTimeoutMillis() == 5000);
  assert(watchdogBackend.feedCalls() == 2);
}

}  // namespace

int main() {
  testCorePlatformUsesInjectedTimeSource();
  testCorePlatformUsesInjectedLogSink();
  testCorePlatformUsesInjectedWatchdogBackend();
  return 0;
}

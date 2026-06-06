#include <assert.h>
#include <string>
#include <string.h>

#include "core/events/event_bus.h"
#include "core/logging/logger.h"
#include "core/scheduler/task_scheduler.h"
#include "core/state/system_state.h"
#include "core/watchdog/watchdog_service.h"
#include "fakes/fake_log_sink.h"
#include "fakes/fake_time_source.h"
#include "fakes/fake_watchdog_backend.h"

namespace {

using reeflow::core::events::EventBus;
using reeflow::core::logging::Logger;
using reeflow::core::scheduler::TaskScheduler;
using reeflow::core::state::currentSystemState;
using reeflow::core::state::resetSystemState;
using reeflow::core::watchdog::WatchdogService;
using reeflow::test::fakes::FakeLogSink;
using reeflow::test::fakes::FakeTimeSource;
using reeflow::test::fakes::FakeWatchdogBackend;

void testInitializeWithFakeBackend() {
  resetSystemState();
  FakeWatchdogBackend backend;
  FakeTimeSource timeSource;
  FakeLogSink sink;
  Logger logger(sink);
  WatchdogService watchdog(backend, timeSource, logger);

  assert(watchdog.initialize(5000));

  assert(watchdog.initialized());
  assert(backend.configureCalls() == 1);
  assert(backend.configuredTimeoutMillis() == 5000);
  assert(currentSystemState().systemHealth.watchdogTriggered == false);
  assert(strcmp(currentSystemState().systemHealth.rebootReason, "UNKNOWN") ==
         0);
  assert(sink.messages().size() == 1);
  assert(sink.messages()[0] == std::string("[INFO] watchdog: initialized\n"));
}

void testDirectFeedUpdatesBackendAndSystemHealth() {
  resetSystemState();
  FakeWatchdogBackend backend;
  FakeTimeSource timeSource;
  FakeLogSink sink;
  Logger logger(sink);
  WatchdogService watchdog(backend, timeSource, logger);

  assert(watchdog.initialize(5000));
  timeSource.setUptimeMillis(1200);
  assert(watchdog.feed());

  assert(backend.feedCalls() == 1);
  assert(currentSystemState().systemHealth.uptime == 1200);
  assert(!currentSystemState().systemHealth.watchdogTriggered);
}

void testSchedulerCanFeedWatchdog() {
  resetSystemState();
  FakeWatchdogBackend backend;
  FakeTimeSource timeSource;
  FakeLogSink sink;
  Logger logger(sink);
  EventBus eventBus;
  WatchdogService watchdog(backend, timeSource, logger);
  TaskScheduler scheduler(timeSource, eventBus);

  assert(watchdog.initialize(5000));
  scheduler.registerTask("watchdog", 1000,
                         reeflow::core::watchdog::feedWatchdogTask,
                         &watchdog);

  timeSource.advanceMillis(1000);
  const reeflow::core::scheduler::SchedulerRunResult result =
      scheduler.runDueTasks();

  assert(result.executedCount == 1);
  assert(result.failedCount == 0);
  assert(backend.feedCalls() == 1);
}

void testSimulatedTriggerUpdatesSystemHealthAndLogs() {
  resetSystemState();
  FakeWatchdogBackend backend;
  FakeTimeSource timeSource;
  FakeLogSink sink;
  Logger logger(sink);
  WatchdogService watchdog(backend, timeSource, logger);

  assert(watchdog.initialize(5000));
  timeSource.setUptimeMillis(3000);
  watchdog.markTriggered("SIMULATED_WATCHDOG");

  assert(currentSystemState().systemHealth.watchdogTriggered);
  assert(currentSystemState().systemHealth.uptime == 3000);
  assert(strcmp(currentSystemState().systemHealth.rebootReason,
                "SIMULATED_WATCHDOG") == 0);
  assert(sink.messages().size() == 2);
  assert(sink.messages()[1] == std::string("[ERROR] watchdog: triggered\n"));
}

void testFeedBeforeInitializeFailsAndLogs() {
  resetSystemState();
  FakeWatchdogBackend backend;
  FakeTimeSource timeSource;
  FakeLogSink sink;
  Logger logger(sink);
  WatchdogService watchdog(backend, timeSource, logger);

  assert(!watchdog.feed());

  assert(backend.feedCalls() == 0);
  assert(sink.messages().size() == 1);
  assert(sink.messages()[0] ==
         std::string("[ERROR] watchdog: feed before initialize\n"));
}

}  // namespace

int main() {
  testInitializeWithFakeBackend();
  testDirectFeedUpdatesBackendAndSystemHealth();
  testSchedulerCanFeedWatchdog();
  testSimulatedTriggerUpdatesSystemHealthAndLogs();
  testFeedBeforeInitializeFailsAndLogs();
  return 0;
}

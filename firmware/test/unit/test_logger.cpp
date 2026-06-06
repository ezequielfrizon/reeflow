#include <assert.h>
#include <string>

#include "core/events/event_bus.h"
#include "core/logging/logger.h"
#include "core/scheduler/task_scheduler.h"
#include "core/state/system_state.h"
#include "fakes/fake_log_sink.h"
#include "fakes/fake_time_source.h"

namespace {

using reeflow::core::events::EventBus;
using reeflow::core::logging::LogLevel;
using reeflow::core::logging::Logger;
using reeflow::core::scheduler::TaskScheduler;
using reeflow::test::fakes::FakeLogSink;
using reeflow::test::fakes::FakeTimeSource;

struct TaskCounter {
  uint8_t runCount;
  bool shouldSucceed;
};

bool runTask(void* context) {
  TaskCounter* counter = static_cast<TaskCounter*>(context);
  counter->runCount += 1;
  return counter->shouldSucceed;
}

void testLogEmissionByLevel() {
  FakeLogSink sink;
  Logger logger(sink);

  logger.debug("test", "debug message");
  logger.info("test", "info message");
  logger.warning("test", "warning message");
  logger.error("test", "error message");

  assert(sink.messages().size() == 4);
  assert(sink.messages()[0] == std::string("[DEBUG] test: debug message\n"));
  assert(sink.messages()[1] == std::string("[INFO] test: info message\n"));
  assert(sink.messages()[2] ==
         std::string("[WARNING] test: warning message\n"));
  assert(sink.messages()[3] == std::string("[ERROR] test: error message\n"));
}

void testLevelFiltering() {
  FakeLogSink sink;
  Logger logger(sink);
  logger.setMinimumLevel(LogLevel::kWarning);

  logger.debug("test", "debug hidden");
  logger.info("test", "info hidden");
  logger.warning("test", "warning visible");
  logger.error("test", "error visible");

  assert(sink.messages().size() == 2);
  assert(sink.messages()[0] ==
         std::string("[WARNING] test: warning visible\n"));
  assert(sink.messages()[1] == std::string("[ERROR] test: error visible\n"));
}

void testOriginFallbackAndSinkInjection() {
  FakeLogSink sink;
  Logger logger(sink);

  logger.info(nullptr, nullptr);

  assert(sink.messages().size() == 1);
  assert(sink.messages()[0] == std::string("[INFO] unknown: \n"));
}

void testSchedulerCanUseLoggerWithoutReplacingEventBus() {
  FakeTimeSource timeSource;
  EventBus eventBus;
  FakeLogSink sink;
  Logger logger(sink);
  TaskScheduler scheduler(timeSource, eventBus, logger);
  TaskCounter failing = {};
  failing.shouldSucceed = false;

  scheduler.registerTask("failing", 1000, runTask, &failing);
  timeSource.advanceMillis(1000);
  const reeflow::core::scheduler::SchedulerRunResult result =
      scheduler.runDueTasks();

  assert(result.executedCount == 1);
  assert(result.failedCount == 1);
  assert(sink.messages().size() == 1);
  assert(sink.messages()[0] ==
         std::string("[ERROR] scheduler: task failed\n"));
}

void testLoggerHasNoCircularDependencyWithEventsOrState() {
  FakeLogSink sink;
  Logger logger(sink);
  EventBus eventBus;
  const reeflow::core::state::SystemState defaultState =
      reeflow::core::state::makeDefaultSystemState();

  logger.info("core", "dependencies ok");

  assert(sink.messages().size() == 1);
  assert(defaultState.system.version == 0);
  const reeflow::core::events::PublishResult result =
      eventBus.publish({});
  assert(result.deliveredCount == 0);
  assert(result.failedCount == 0);
}

}  // namespace

int main() {
  testLogEmissionByLevel();
  testLevelFiltering();
  testOriginFallbackAndSinkInjection();
  testSchedulerCanUseLoggerWithoutReplacingEventBus();
  testLoggerHasNoCircularDependencyWithEventsOrState();
  return 0;
}

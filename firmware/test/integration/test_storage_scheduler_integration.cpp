#include <assert.h>

#include "app/core_app.h"
#include "config/config_manager.h"
#include "core/events/event_bus.h"
#include "core/platform/core_platform.h"
#include "fakes/fake_lighting_pwm_controller.h"
#include "fakes/fake_log_sink.h"
#include "fakes/fake_relay_controller.h"
#include "fakes/fake_storage_backend.h"
#include "fakes/fake_temperature_sensor.h"
#include "fakes/fake_time_source.h"
#include "fakes/fake_watchdog_backend.h"
#include "fakes/fake_water_level_sensor.h"
#include "storage/storage_schema.h"
#include "storage/storage_service.h"

namespace {

using reeflow::app::CoreApp;
using reeflow::app::kStorageFlushIntervalMillis;
using reeflow::config::ConfigManager;
using reeflow::core::events::ConfigDomain;
using reeflow::core::events::EventBus;
using reeflow::core::platform::CorePlatform;
using reeflow::storage::StorageModeStore;
using reeflow::storage::StorageService;
using reeflow::storage::storageKeyForDomain;
using reeflow::storage::storageNamespaceForDomain;
using reeflow::test::fakes::FakeLightingPwmController;
using reeflow::test::fakes::FakeLogSink;
using reeflow::test::fakes::FakeRelayController;
using reeflow::test::fakes::FakeStorageBackend;
using reeflow::test::fakes::FakeTemperatureSensor;
using reeflow::test::fakes::FakeTimeSource;
using reeflow::test::fakes::FakeWatchdogBackend;
using reeflow::test::fakes::FakeWaterLevelSensor;

struct TestContext {
  FakeTimeSource timeSource;
  FakeLogSink logSink;
  FakeWatchdogBackend watchdogBackend;
  CorePlatform platform;
  EventBus eventBus;
  ConfigManager configManager;
  FakeTemperatureSensor temperatureSensor;
  FakeWaterLevelSensor waterLevelSensor;
  FakeRelayController relayController;
  FakeLightingPwmController lightingController;
  FakeStorageBackend storageBackend;
  StorageService storageService;
  StorageModeStore modeStore;
  CoreApp app;

  TestContext()
      : platform(timeSource, logSink, watchdogBackend),
        configManager(eventBus),
        storageService(storageBackend, eventBus),
        modeStore(storageService),
        app(platform, eventBus, configManager, temperatureSensor,
            waterLevelSensor, relayController, lightingController, modeStore,
            &storageService) {}
};

uint16_t writeCount(FakeStorageBackend& backend, ConfigDomain domain) {
  return backend.writeCount(storageNamespaceForDomain(domain),
                            storageKeyForDomain(domain));
}

void testFlushRunsOnlyAfterIntervalAndWritesDirtyDomains() {
  TestContext context;
  assert(context.app.setup());

  reeflow::config::AtoConfig ato = context.configManager.ato();
  ato.enabled = true;
  assert(context.configManager.updateAto(ato));
  assert(context.storageService.dirtyCount() == 1);

  const uint16_t beforeWrites = writeCount(context.storageBackend,
                                           ConfigDomain::kAto);
  context.timeSource.advanceMillis(kStorageFlushIntervalMillis - 1);
  auto result = context.app.loopOnce();
  (void)result;
  assert(writeCount(context.storageBackend, ConfigDomain::kAto) ==
         beforeWrites);

  context.timeSource.advanceMillis(1);
  result = context.app.loopOnce();
  (void)result;
  assert(writeCount(context.storageBackend, ConfigDomain::kAto) ==
         beforeWrites + 1);
  assert(context.storageService.dirtyCount() == 0);
}

void testFlushCoalescesMultipleAcceptedChangesAndSkipsRejectedOnes() {
  TestContext context;
  assert(context.app.setup());

  reeflow::config::TimersConfig timers = context.configManager.timers();
  timers.feedingDurationSeconds = 100;
  assert(context.configManager.updateTimers(timers));
  timers.feedingDurationSeconds = 200;
  assert(context.configManager.updateTimers(timers));

  reeflow::config::TimersConfig invalidTimers = timers;
  invalidTimers.feedingDurationSeconds = 90000;
  assert(!context.configManager.updateTimers(invalidTimers));

  assert(context.storageService.dirtyCount() == 1);
  const uint16_t beforeWrites = writeCount(context.storageBackend,
                                           ConfigDomain::kTimers);
  context.timeSource.advanceMillis(kStorageFlushIntervalMillis);
  const auto result = context.app.loopOnce();

  (void)result;
  assert(writeCount(context.storageBackend, ConfigDomain::kTimers) ==
         beforeWrites + 1);
  assert(context.storageService.dirtyCount() == 0);

  ConfigManager restoredConfig(context.eventBus);
  restoredConfig.loadDefaults();
  assert(context.storageService.restoreTimers(restoredConfig) ==
         reeflow::storage::StorageResult::kSuccess);
  assert(restoredConfig.timers().feedingDurationSeconds == 200);
}

void testFlushFailureIsReportedWithoutCorruptingConfig() {
  TestContext context;
  assert(context.app.setup());

  reeflow::config::TemperatureConfig temperature =
      context.configManager.temperature();
  temperature.targetTemperature = 26.0F;
  assert(context.configManager.updateTemperature(temperature));
  context.storageBackend.simulateWriteFailure();

  context.timeSource.advanceMillis(kStorageFlushIntervalMillis);
  const auto result = context.app.loopOnce();

  assert(result.failedCount >= 1);
  assert(context.configManager.temperature().targetTemperature == 26.0F);
  assert(context.storageService.dirtyCount() == 1);
}

}  // namespace

int main() {
  testFlushRunsOnlyAfterIntervalAndWritesDirtyDomains();
  testFlushCoalescesMultipleAcceptedChangesAndSkipsRejectedOnes();
  testFlushFailureIsReportedWithoutCorruptingConfig();
  return 0;
}

#include <assert.h>
#include <string.h>

#include "app/core_app.h"
#include "config/config_manager.h"
#include "core/events/event_bus.h"
#include "core/platform/core_platform.h"
#include "core/state/system_state.h"
#include "fakes/fake_lighting_pwm_controller.h"
#include "fakes/fake_log_sink.h"
#include "fakes/fake_relay_controller.h"
#include "fakes/fake_storage_backend.h"
#include "fakes/fake_temperature_sensor.h"
#include "fakes/fake_time_source.h"
#include "fakes/fake_watchdog_backend.h"
#include "fakes/fake_water_level_sensor.h"
#include "modules/lighting/lighting_profile.h"
#include "storage/storage_schema.h"
#include "storage/storage_service.h"

namespace {

using reeflow::app::CoreApp;
using reeflow::config::ConfigManager;
using reeflow::core::events::EventBus;
using reeflow::core::platform::CorePlatform;
using reeflow::core::state::OperationalMode;
using reeflow::core::state::LightingMode;
using reeflow::modules::lighting::makeDefaultLightingProfile;
using reeflow::storage::LightingPersistedState;
using reeflow::storage::StorageModeStore;
using reeflow::storage::StorageResult;
using reeflow::storage::StorageService;
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

LightingPersistedState makeLightingState() {
  LightingPersistedState lighting = {};
  lighting.config.mode = LightingMode::kAutomatic;
  lighting.config.sunriseEnabled = true;
  lighting.config.sunsetEnabled = true;
  lighting.config.acclimationEnabled = false;
  lighting.config.startMinuteOfDay = 480;
  lighting.config.endMinuteOfDay = 1200;
  lighting.config.maxIntensityPercent = 85;
  lighting.config.white = {true, 180};
  lighting.config.blue = {true, 190};
  lighting.config.royalBlue = {true, 200};
  lighting.config.uv = {true, 90};
  lighting.profileCount = 1;
  lighting.currentProfileIndex = 0;
  lighting.profiles[0] = makeDefaultLightingProfile();
  strncpy(lighting.profiles[0].name, "boot-reef",
          sizeof(lighting.profiles[0].name) - 1);
  lighting.profiles[0].name[sizeof(lighting.profiles[0].name) - 1] = '\0';
  lighting.profiles[0].white.maxDuty = 170;
  lighting.profiles[0].uv.maxDuty = 80;
  return lighting;
}

void testBootRestoresConfigBeforeFunctionalTasks() {
  TestContext context;

  reeflow::config::WifiConfig wifi = {};
  wifi.enabled = true;
  strncpy(wifi.ssid, "reef-wifi", sizeof(wifi.ssid) - 1);
  assert(context.storageService.saveWifi(wifi) == StorageResult::kSuccess);

  reeflow::config::MqttConfig mqtt = {};
  mqtt.enabled = true;
  strncpy(mqtt.host, "broker.local", sizeof(mqtt.host) - 1);
  strncpy(mqtt.clientId, "reef", sizeof(mqtt.clientId) - 1);
  mqtt.port = 1883;
  mqtt.heartbeatIntervalMillis = 30000;
  assert(context.storageService.saveMqtt(mqtt) == StorageResult::kSuccess);

  reeflow::config::AtoConfig ato = {};
  ato.enabled = true;
  ato.minimumLevel = 30;
  ato.maximumLevel = 70;
  ato.timeoutMillis = 90000;
  ato.cooldownMillis = 400000;
  assert(context.storageService.saveAto(ato) == StorageResult::kSuccess);

  reeflow::config::TimersConfig timers = {};
  timers.feedingDurationSeconds = 123;
  timers.tpaDurationSeconds = 456;
  timers.maintenanceDurationSeconds = 789;
  assert(context.storageService.saveTimers(timers) ==
         StorageResult::kSuccess);
  assert(context.storageService.saveMode(OperationalMode::kTpa) ==
         StorageResult::kSuccess);
  assert(context.storageService.saveLighting(makeLightingState()) ==
         StorageResult::kSuccess);

  assert(context.app.setup());

  assert(context.configManager.wifi().enabled);
  assert(strcmp(context.configManager.wifi().ssid, "reef-wifi") == 0);
  assert(context.configManager.mqtt().enabled);
  assert(strcmp(context.configManager.mqtt().host, "broker.local") == 0);
  assert(context.configManager.ato().enabled);
  assert(context.configManager.ato().minimumLevel == 30);
  assert(context.configManager.timers().feedingDurationSeconds == 123);
  assert(context.configManager.lighting().mode == LightingMode::kAutomatic);
  assert(strcmp(reeflow::core::state::currentSystemState().lighting
                    .currentProfile,
                "boot-reef") == 0);
  assert(reeflow::core::state::currentSystemState().modes.currentMode ==
         OperationalMode::kTpa);
  assert(!reeflow::core::state::currentSystemState().relays.recalque.enabled);
  assert(!reeflow::core::state::currentSystemState().relays.atoPump.enabled);
  assert(context.relayController.allOffCallCount() == 1);
  assert(context.lightingController.allOffCallCount() == 1);
  assert(!reeflow::core::state::currentSystemState().network.wifiConnected);
  assert(!reeflow::core::state::currentSystemState().network.mqttConnected);
  assert(context.storageService.dirtyCount() == 0);
}

void testInvalidBootPayloadKeepsDefaultsAndDoesNotBlockSetup() {
  TestContext context;
  reeflow::config::AtoConfig ato = {};
  ato.enabled = true;
  ato.minimumLevel = 10;
  ato.maximumLevel = 90;
  ato.timeoutMillis = 60000;
  ato.cooldownMillis = 300000;
  assert(context.storageService.saveAto(ato) == StorageResult::kSuccess);
  context.storageBackend.simulateCorruptedPayload(
      reeflow::storage::storageNamespaceForDomain(
          reeflow::core::events::ConfigDomain::kAto),
      reeflow::storage::storageKeyForDomain(
          reeflow::core::events::ConfigDomain::kAto));

  assert(context.app.setup());

  assert(!context.configManager.ato().enabled);
  assert(context.configManager.ato().minimumLevel == 20);
  assert(context.relayController.allOffCallCount() == 1);
  assert(context.storageService.dirtyCount() == 0);
}

}  // namespace

int main() {
  testBootRestoresConfigBeforeFunctionalTasks();
  testInvalidBootPayloadKeepsDefaultsAndDoesNotBlockSetup();
  return 0;
}

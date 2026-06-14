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
#include "modules/relays/relay_types.h"
#include "resilience/resilience_report.h"
#include "resilience/resilience_scenarios.h"
#include "storage/storage_service.h"

namespace {

using reeflow::app::CoreApp;
using reeflow::config::ConfigManager;
using reeflow::core::events::Event;
using reeflow::core::events::EventBus;
using reeflow::core::events::EventType;
using reeflow::core::platform::CorePlatform;
using reeflow::core::state::AtoState;
using reeflow::core::state::AtoStatus;
using reeflow::core::state::LightingMode;
using reeflow::core::state::OperationalMode;
using reeflow::core::state::RelaySource;
using reeflow::core::state::SystemHealthState;
using reeflow::core::state::TemperatureState;
using reeflow::core::state::TemperatureStatus;
using reeflow::core::state::WaterLevelState;
using reeflow::core::state::WaterLevelStatus;
using reeflow::core::state::currentSystemState;
using reeflow::core::state::updateAtoState;
using reeflow::core::state::updateLightingState;
using reeflow::core::state::updateSystemHealthState;
using reeflow::core::state::updateTemperatureState;
using reeflow::core::state::updateWaterLevelState;
using reeflow::modules::lighting::LightingChannel;
using reeflow::modules::lighting::makeDefaultLightingProfile;
using reeflow::modules::relays::RelayDesiredState;
using reeflow::modules::relays::RelayId;
using reeflow::resilience::ResilienceReport;
using reeflow::resilience::ResilienceResult;
using reeflow::resilience::ResilienceScenarioId;
using reeflow::resilience::findResilienceScenario;
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

struct EventRecorder {
  static constexpr size_t kMaxEvents = 32;
  Event events[kMaxEvents];
  size_t count;

  static bool capture(const Event& event, void* context) {
    EventRecorder* recorder = static_cast<EventRecorder*>(context);
    if (recorder == nullptr || recorder->count >= kMaxEvents) {
      return false;
    }

    recorder->events[recorder->count++] = event;
    return true;
  }

  bool has(EventType type) const {
    for (size_t index = 0; index < count; ++index) {
      if (events[index].type == type) {
        return true;
      }
    }
    return false;
  }
};

struct BootContext {
  FakeTimeSource timeSource;
  FakeLogSink logSink;
  FakeWatchdogBackend watchdogBackend;
  CorePlatform platform;
  EventBus eventBus;
  EventRecorder events;
  ConfigManager configManager;
  FakeTemperatureSensor temperatureSensor;
  FakeWaterLevelSensor waterLevelSensor;
  FakeRelayController relayController;
  FakeLightingPwmController lightingController;
  StorageService storageService;
  StorageModeStore modeStore;
  CoreApp app;

  explicit BootContext(FakeStorageBackend& storageBackend)
      : platform(timeSource, logSink, watchdogBackend),
        configManager(eventBus),
        storageService(storageBackend, eventBus),
        modeStore(storageService),
        app(platform, eventBus, configManager, temperatureSensor,
            waterLevelSensor, relayController, lightingController, modeStore,
            &storageService) {
    eventBus.subscribe(EventType::kSystemStateChanged, EventRecorder::capture,
                       &events);
    eventBus.subscribe(EventType::kConfigRestored, EventRecorder::capture,
                       &events);
    eventBus.subscribe(EventType::kAlertRaised, EventRecorder::capture,
                       &events);
  }

  void advanceAndRun(uint32_t millis) {
    timeSource.advanceMillis(millis);
    const auto result = app.loopOnce();
    (void)result;
  }
};

LightingPersistedState makeLightingState() {
  LightingPersistedState lighting = {};
  lighting.config.mode = LightingMode::kAutomatic;
  lighting.config.sunriseEnabled = true;
  lighting.config.sunsetEnabled = true;
  lighting.config.acclimationEnabled = false;
  lighting.config.startMinuteOfDay = 420;
  lighting.config.endMinuteOfDay = 1140;
  lighting.config.maxIntensityPercent = 80;
  lighting.config.white = {true, 180};
  lighting.config.blue = {true, 190};
  lighting.config.royalBlue = {true, 200};
  lighting.config.uv = {true, 90};
  lighting.profileCount = 1;
  lighting.currentProfileIndex = 0;
  lighting.profiles[0] = makeDefaultLightingProfile();
  strncpy(lighting.profiles[0].name, "resilience-boot",
          sizeof(lighting.profiles[0].name) - 1);
  lighting.profiles[0].name[sizeof(lighting.profiles[0].name) - 1] = '\0';
  lighting.profiles[0].white.maxDuty = 170;
  lighting.profiles[0].blue.maxDuty = 175;
  lighting.profiles[0].royalBlue.maxDuty = 180;
  lighting.profiles[0].uv.maxDuty = 70;
  return lighting;
}

void seedPersistentState(StorageService& storage) {
  reeflow::config::AtoConfig ato = {};
  ato.enabled = true;
  ato.minimumLevel = 25;
  ato.maximumLevel = 75;
  ato.timeoutMillis = 90000;
  ato.cooldownMillis = 300000;
  assert(storage.saveAto(ato) == StorageResult::kSuccess);
  assert(storage.saveMode(OperationalMode::kTpa) == StorageResult::kSuccess);
  assert(storage.saveLighting(makeLightingState()) == StorageResult::kSuccess);
}

void makeRuntimeStateDirty(CoreApp& app) {
  assert(app.setLocalRelay(RelayId::kAtoPump, RelayDesiredState::kOn) ==
         reeflow::modules::relays::RelayCommandResult::kSuccess);

  TemperatureState temperature = currentSystemState().temperature;
  temperature.currentTemperature = 26.5F;
  temperature.status = TemperatureStatus::kNormal;
  temperature.lastUpdate = 1234;
  updateTemperatureState(temperature);

  WaterLevelState waterLevel = currentSystemState().waterLevel;
  waterLevel.currentLevel = 45;
  waterLevel.status = WaterLevelStatus::kNormal;
  waterLevel.lastUpdate = 1234;
  updateWaterLevelState(waterLevel);

  AtoState ato = currentSystemState().ato;
  ato.enabled = true;
  ato.status = AtoStatus::kRefilling;
  ato.pumpRunning = true;
  ato.lastActivation = 1234;
  updateAtoState(ato);

  auto lighting = currentSystemState().lighting;
  lighting.white.currentPWM = 177;
  lighting.blue.currentPWM = 155;
  lighting.royalBlue.currentPWM = 144;
  lighting.uv.currentPWM = 66;
  updateLightingState(lighting);
}

void markUnexpectedReboot(CoreApp& app) {
  app.watchdog().markTriggered("watchdog");
}

bool hasActiveUnexpectedRebootAlert() {
  const auto& alerts = currentSystemState().alerts;
  for (size_t index = 0; index < reeflow::core::state::kMaxActiveAlerts;
       ++index) {
    if (alerts.activeAlerts[index].active &&
        strcmp(alerts.activeAlerts[index].name, "UNEXPECTED_REBOOT") == 0) {
      return true;
    }
  }
  return false;
}

bool bootIsSafe(const BootContext& context, bool expectRebootAlert) {
  const auto& state = currentSystemState();
  const bool relaysSafe = !state.relays.recalque.enabled &&
                          !state.relays.heater.enabled &&
                          !state.relays.atoPump.enabled &&
                          !state.relays.reserve.enabled;
  const bool atoSafe =
      !state.ato.pumpRunning && state.ato.status != AtoStatus::kRefilling;
  const bool volatileSensorsCleared =
      state.temperature.status == TemperatureStatus::kSensorOffline &&
      state.temperature.lastUpdate == 0 &&
      state.waterLevel.status == WaterLevelStatus::kSensorOffline &&
      state.waterLevel.lastUpdate == 0;
  const bool volatileLightingCleared =
      state.lighting.white.currentPWM == 0 && state.lighting.blue.currentPWM == 0 &&
      state.lighting.royalBlue.currentPWM == 0 &&
      state.lighting.uv.currentPWM == 0;
  const bool persistedConfigRestored =
      context.configManager.ato().enabled &&
      context.configManager.ato().minimumLevel == 25 &&
      context.configManager.lighting().mode == LightingMode::kAutomatic &&
      strcmp(state.lighting.currentProfile, "resilience-boot") == 0 &&
      state.modes.currentMode == OperationalMode::kTpa;
  const bool coreServicesReady =
      context.watchdogBackend.configureCalls() == 1 &&
      context.relayController.allOffCallCount() == 1 &&
      context.lightingController.allOffCallCount() == 1 &&
      context.events.has(EventType::kConfigRestored);
  const bool rebootAlertOk = !expectRebootAlert ||
                             (context.events.has(EventType::kAlertRaised) &&
                              hasActiveUnexpectedRebootAlert());

  assert(relaysSafe);
  assert(atoSafe);
  assert(volatileSensorsCleared);
  assert(volatileLightingCleared);
  assert(persistedConfigRestored);
  assert(coreServicesReady);
  if (expectRebootAlert) {
    assert(hasActiveUnexpectedRebootAlert());
    assert(context.events.has(EventType::kAlertRaised));
  }
  assert(rebootAlertOk);

  return relaysSafe && atoSafe && volatileSensorsCleared &&
         volatileLightingCleared && persistedConfigRestored &&
         coreServicesReady && rebootAlertOk;
}

ResilienceResult runUnexpectedRebootScenario(ResilienceReport& report) {
  FakeStorageBackend storageBackend;
  BootContext firstBoot(storageBackend);
  seedPersistentState(firstBoot.storageService);

  const auto* scenario =
      findResilienceScenario(ResilienceScenarioId::kUnexpectedReboot);
  assert(scenario != nullptr);
  assert(firstBoot.app.setup());
  assert(report.beginScenario(*scenario, firstBoot.timeSource.uptimeMillis(),
                              currentSystemState()));

  makeRuntimeStateDirty(firstBoot.app);
  firstBoot.advanceAndRun(5000);

  BootContext rebooted(storageBackend);
  assert(rebooted.app.setup());
  rebooted.timeSource.advanceMillis(1000);
  markUnexpectedReboot(rebooted.app);
  const bool rebootAlertObserved =
      rebooted.events.has(EventType::kAlertRaised) &&
      hasActiveUnexpectedRebootAlert();
  rebooted.advanceAndRun(5000);

  const ResilienceResult result =
      rebootAlertObserved && bootIsSafe(rebooted, false)
          ? ResilienceResult::kPassed
          : ResilienceResult::kFailed;
  assert(report.recordSnapshot(scenario->id, rebooted.timeSource.uptimeMillis(),
                               currentSystemState()));
  assert(report.completeScenario(scenario->id, result,
                                 rebooted.timeSource.uptimeMillis(),
                                 currentSystemState()));
  return result;
}

ResilienceResult runPowerLossScenario(ResilienceReport& report) {
  FakeStorageBackend storageBackend;
  BootContext firstBoot(storageBackend);
  seedPersistentState(firstBoot.storageService);

  const auto* scenario =
      findResilienceScenario(ResilienceScenarioId::kSimulatedPowerLoss);
  assert(scenario != nullptr);
  assert(firstBoot.app.setup());
  assert(report.beginScenario(*scenario, firstBoot.timeSource.uptimeMillis(),
                              currentSystemState()));

  makeRuntimeStateDirty(firstBoot.app);

  BootContext afterPowerLoss(storageBackend);
  assert(afterPowerLoss.app.setup());
  afterPowerLoss.advanceAndRun(5000);

  const ResilienceResult result =
      bootIsSafe(afterPowerLoss, false) ? ResilienceResult::kPassed
                                        : ResilienceResult::kFailed;
  assert(report.recordSnapshot(scenario->id,
                               afterPowerLoss.timeSource.uptimeMillis(),
                               currentSystemState()));
  assert(report.completeScenario(scenario->id, result,
                                 afterPowerLoss.timeSource.uptimeMillis(),
                                 currentSystemState()));
  return result;
}

void testBootAndPowerScenarios() {
  ResilienceReport report;
  assert(runUnexpectedRebootScenario(report) == ResilienceResult::kPassed);
  assert(runPowerLossScenario(report) == ResilienceResult::kPassed);

  const auto summary = report.summary();
  assert(summary.scenarioCount == 2);
  assert(summary.passedCount == 2);
  assert(summary.failedCount == 0);
  assert(summary.notRunCount == 0);
}

}  // namespace

int main() {
  testBootAndPowerScenarios();
  return 0;
}

#include <assert.h>
#include <string.h>

#include "app/core_app.h"
#include "config/config_manager.h"
#include "core/events/event_bus.h"
#include "core/platform/core_platform.h"
#include "core/state/system_state.h"
#include "fakes/fake_lighting_pwm_controller.h"
#include "fakes/fake_log_sink.h"
#include "fakes/fake_mode_store.h"
#include "fakes/fake_relay_controller.h"
#include "fakes/fake_resilience_sensors.h"
#include "fakes/fake_time_source.h"
#include "fakes/fake_watchdog_backend.h"
#include "modules/relays/relay_types.h"
#include "resilience/resilience_report.h"
#include "resilience/resilience_scenarios.h"

namespace {

using reeflow::app::CoreApp;
using reeflow::config::ConfigManager;
using reeflow::core::events::Event;
using reeflow::core::events::EventBus;
using reeflow::core::events::EventType;
using reeflow::core::platform::CorePlatform;
using reeflow::core::state::AtoStatus;
using reeflow::core::state::TemperatureStatus;
using reeflow::core::state::WaterLevelStatus;
using reeflow::core::state::currentSystemState;
using reeflow::resilience::ResilienceReport;
using reeflow::resilience::ResilienceResult;
using reeflow::resilience::ResilienceScenarioId;
using reeflow::resilience::findResilienceScenario;
using reeflow::test::fakes::FakeLightingPwmController;
using reeflow::test::fakes::FakeLogSink;
using reeflow::test::fakes::FakeModeStore;
using reeflow::test::fakes::FakeRelayController;
using reeflow::test::fakes::FakeResilienceSensors;
using reeflow::test::fakes::FakeTimeSource;
using reeflow::test::fakes::FakeWatchdogBackend;

struct EventRecorder {
  static constexpr size_t kMaxEvents = 48;
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

struct SensorContext {
  FakeTimeSource timeSource;
  FakeLogSink logSink;
  FakeWatchdogBackend watchdogBackend;
  CorePlatform platform;
  EventBus eventBus;
  EventRecorder events;
  ConfigManager configManager;
  FakeResilienceSensors sensors;
  FakeRelayController relayController;
  FakeLightingPwmController lightingController;
  FakeModeStore modeStore;
  CoreApp app;

  SensorContext()
      : platform(timeSource, logSink, watchdogBackend),
        configManager(eventBus),
        app(platform, eventBus, configManager, sensors.temperature(),
            sensors.waterLevel(), relayController, lightingController,
            modeStore) {
    eventBus.subscribe(EventType::kTemperatureSensorOffline,
                       EventRecorder::capture, &events);
    eventBus.subscribe(EventType::kTemperatureSensorRecovered,
                       EventRecorder::capture, &events);
    eventBus.subscribe(EventType::kWaterLevelSensorOffline,
                       EventRecorder::capture, &events);
    eventBus.subscribe(EventType::kWaterLevelSensorRecovered,
                       EventRecorder::capture, &events);
    eventBus.subscribe(EventType::kAtoSensorOffline, EventRecorder::capture,
                       &events);
    eventBus.subscribe(EventType::kAtoRecovered, EventRecorder::capture,
                       &events);
  }

  void setupOnline() {
    sensors.startOnline();
    assert(app.setup());

    reeflow::config::AtoConfig ato = configManager.ato();
    ato.enabled = true;
    ato.minimumLevel = 20;
    ato.maximumLevel = 80;
    assert(configManager.updateAto(ato));
  }

  void advanceAndRun(uint32_t millis) {
    timeSource.advanceMillis(millis);
    const auto result = app.loopOnce();
    (void)result;
  }
};

bool activeAlertNamed(const char* name) {
  const auto& alerts = currentSystemState().alerts;
  for (size_t index = 0; index < reeflow::core::state::kMaxActiveAlerts;
       ++index) {
    if (alerts.activeAlerts[index].active &&
        strcmp(alerts.activeAlerts[index].name, name) == 0) {
      return true;
    }
  }
  return false;
}

bool actuatorsRemainUnchanged(const SensorContext& context) {
  const auto& state = currentSystemState();
  return !state.relays.recalque.enabled && !state.relays.heater.enabled &&
         !state.relays.atoPump.enabled && !state.relays.reserve.enabled &&
         !state.ato.pumpRunning &&
         state.modes.currentMode == reeflow::core::state::OperationalMode::kNormal &&
         state.lighting.white.currentPWM == 0 &&
         state.lighting.blue.currentPWM == 0 &&
         state.lighting.royalBlue.currentPWM == 0 &&
         state.lighting.uv.currentPWM == 0 &&
         context.relayController.allOffCallCount() == 1;
}

ResilienceResult runTemperatureSensorOfflineScenario(
    ResilienceReport& report) {
  SensorContext context;
  const auto* scenario =
      findResilienceScenario(ResilienceScenarioId::kTemperatureSensorOffline);
  assert(scenario != nullptr);
  context.setupOnline();
  assert(report.beginScenario(*scenario, context.timeSource.uptimeMillis(),
                              currentSystemState()));

  context.advanceAndRun(5000);
  assert(currentSystemState().temperature.status ==
         TemperatureStatus::kNormal);

  context.sensors.failTemperature();
  context.advanceAndRun(35000);
  assert(currentSystemState().temperature.status ==
         TemperatureStatus::kSensorOffline);
  assert(context.events.has(EventType::kTemperatureSensorOffline));
  assert(activeAlertNamed("TEMPERATURE_SENSOR_OFFLINE"));
  assert(actuatorsRemainUnchanged(context));

  context.sensors.recoverTemperature();
  context.advanceAndRun(5000);
  assert(currentSystemState().temperature.status ==
         TemperatureStatus::kNormal);
  assert(context.events.has(EventType::kTemperatureSensorRecovered));
  assert(strcmp(currentSystemState().alerts.lastRecovery,
                "TEMPERATURE_SENSOR_RECOVERED") == 0);
  assert(actuatorsRemainUnchanged(context));

  const ResilienceResult result =
      currentSystemState().temperature.status == TemperatureStatus::kNormal &&
              actuatorsRemainUnchanged(context)
          ? ResilienceResult::kPassed
          : ResilienceResult::kFailed;
  assert(report.completeScenario(scenario->id, result,
                                 context.timeSource.uptimeMillis(),
                                 currentSystemState()));
  return result;
}

ResilienceResult runWaterLevelSensorOfflineScenario(
    ResilienceReport& report) {
  SensorContext context;
  const auto* scenario =
      findResilienceScenario(ResilienceScenarioId::kWaterLevelSensorOffline);
  assert(scenario != nullptr);
  context.setupOnline();
  assert(report.beginScenario(*scenario, context.timeSource.uptimeMillis(),
                              currentSystemState()));

  context.advanceAndRun(5000);
  assert(currentSystemState().waterLevel.status == WaterLevelStatus::kNormal);

  context.sensors.failWaterLevel();
  context.advanceAndRun(35000);
  assert(currentSystemState().waterLevel.status ==
         WaterLevelStatus::kSensorOffline);
  assert(currentSystemState().ato.status == AtoStatus::kSensorOffline);
  assert(context.events.has(EventType::kWaterLevelSensorOffline));
  assert(context.events.has(EventType::kAtoSensorOffline));
  assert(activeAlertNamed("ATO_SENSOR_OFFLINE"));
  assert(actuatorsRemainUnchanged(context));

  context.sensors.recoverWaterLevel();
  context.advanceAndRun(5000);
  assert(currentSystemState().waterLevel.status == WaterLevelStatus::kNormal);
  assert(currentSystemState().ato.status == AtoStatus::kNormal);
  assert(context.events.has(EventType::kWaterLevelSensorRecovered));
  assert(context.events.has(EventType::kAtoRecovered));
  assert(strcmp(currentSystemState().alerts.lastRecovery,
                "ATO_SENSOR_RECOVERED") == 0);
  assert(actuatorsRemainUnchanged(context));

  const ResilienceResult result =
      currentSystemState().waterLevel.status == WaterLevelStatus::kNormal &&
              currentSystemState().ato.status == AtoStatus::kNormal &&
              actuatorsRemainUnchanged(context)
          ? ResilienceResult::kPassed
          : ResilienceResult::kFailed;
  assert(report.completeScenario(scenario->id, result,
                                 context.timeSource.uptimeMillis(),
                                 currentSystemState()));
  return result;
}

void testSensorOfflineResilience() {
  ResilienceReport report;
  assert(runTemperatureSensorOfflineScenario(report) ==
         ResilienceResult::kPassed);
  assert(runWaterLevelSensorOfflineScenario(report) ==
         ResilienceResult::kPassed);

  const auto summary = report.summary();
  assert(summary.scenarioCount == 2);
  assert(summary.passedCount == 2);
  assert(summary.failedCount == 0);
  assert(summary.notRunCount == 0);
}

}  // namespace

int main() {
  testSensorOfflineResilience();
  return 0;
}

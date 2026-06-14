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
#include "modules/modes/mode_types.h"
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
using reeflow::core::state::NetworkState;
using reeflow::core::state::RelaySource;
using reeflow::core::state::WaterLevelStatus;
using reeflow::core::state::currentSystemState;
using reeflow::core::state::updateNetworkState;
using reeflow::modules::modes::FEEDING;
using reeflow::modules::modes::MAINTENANCE;
using reeflow::modules::modes::OperationalMode;
using reeflow::modules::modes::TPA;
using reeflow::modules::relays::RelayDesiredState;
using reeflow::modules::relays::RelayId;
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
  size_t count = 0;

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

struct AtoStuckContext {
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

  AtoStuckContext()
      : platform(timeSource, logSink, watchdogBackend),
        configManager(eventBus),
        app(platform, eventBus, configManager, sensors.temperature(),
            sensors.waterLevel(), relayController, lightingController,
            modeStore) {
    eventBus.subscribe(EventType::kAtoStart, EventRecorder::capture, &events);
    eventBus.subscribe(EventType::kAtoTimeout, EventRecorder::capture,
                       &events);
    eventBus.subscribe(EventType::kAtoRecovered, EventRecorder::capture,
                       &events);
    eventBus.subscribe(EventType::kAlertRaised, EventRecorder::capture,
                       &events);
    eventBus.subscribe(EventType::kAlertRecovered, EventRecorder::capture,
                       &events);
  }

  void setupReady() {
    sensors.startOnline();
    assert(app.setup());

    reeflow::config::AtoConfig ato = configManager.ato();
    ato.enabled = true;
    ato.minimumLevel = 20;
    ato.maximumLevel = 80;
    ato.timeoutMillis = 60000;
    ato.cooldownMillis = 300000;
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

void setConnectivityOffline() {
  NetworkState network = currentSystemState().network;
  network.wifiConnected = false;
  network.mqttConnected = false;
  network.internetAvailable = false;
  updateNetworkState(network);
}

void assertPumpOff(const AtoStuckContext& context) {
  assert(!currentSystemState().ato.pumpRunning);
  assert(!currentSystemState().relays.atoPump.enabled);
  assert(context.relayController.state(RelayId::kAtoPump) ==
         RelayDesiredState::kOff);
}

void startRefill(AtoStuckContext& context) {
  context.sensors.waterLevel().setLowLevel();
  context.advanceAndRun(5000);

  assert(currentSystemState().waterLevel.status == WaterLevelStatus::kLow);
  assert(currentSystemState().ato.status == AtoStatus::kRefilling);
  assert(currentSystemState().ato.pumpRunning);
  assert(currentSystemState().relays.atoPump.enabled);
  assert(currentSystemState().relays.atoPump.source ==
         RelaySource::kAutomation);
  assert(context.events.has(EventType::kAtoStart));
}

void assertTimeoutAndCooldown(AtoStuckContext& context) {
  const size_t callsBeforeTimeout = context.relayController.recordedCallCount();
  context.advanceAndRun(60000);

  assert(currentSystemState().ato.status == AtoStatus::kTimeout);
  assert(currentSystemState().ato.timeoutCounter == 1);
  assert(currentSystemState().ato.lastCompletion == context.timeSource.uptimeMillis());
  assertPumpOff(context);
  assert(currentSystemState().relays.atoPump.source == RelaySource::kFailsafe);
  assert(context.relayController.recordedCallCount() > callsBeforeTimeout);
  assert(context.events.has(EventType::kAtoTimeout));
  assert(activeAlertNamed("ATO_TIMEOUT"));

  const uint32_t timeoutCompletion = currentSystemState().ato.lastCompletion;
  const size_t callsAfterTimeout = context.relayController.recordedCallCount();
  context.sensors.waterLevel().setLowLevel();
  context.advanceAndRun(1000);

  assert(currentSystemState().ato.status == AtoStatus::kTimeout);
  assert(currentSystemState().ato.lastCompletion == timeoutCompletion);
  assertPumpOff(context);
  assert(context.relayController.recordedCallCount() == callsAfterTimeout);
}

void recoverAto(AtoStuckContext& context) {
  context.sensors.waterLevel().setValidLevel(55);
  context.advanceAndRun(5000);

  assert(currentSystemState().waterLevel.status == WaterLevelStatus::kNormal);
  assert(currentSystemState().ato.status == AtoStatus::kNormal);
  assertPumpOff(context);
  assert(context.events.has(EventType::kAtoRecovered));
  assert(strcmp(currentSystemState().alerts.lastRecovery, "ATO_RECOVERED") ==
         0);
}

void assertModeBlocksAto(OperationalMode mode) {
  AtoStuckContext context;
  context.setupReady();
  assert(context.app.requestMode(mode) ==
         reeflow::modules::modes::ModeServiceResult::kSuccess);

  context.sensors.waterLevel().setLowLevel();
  context.advanceAndRun(5000);

  assert(currentSystemState().waterLevel.status == WaterLevelStatus::kLow);
  assertPumpOff(context);
  assert(currentSystemState().ato.status == AtoStatus::kDisabled ||
         currentSystemState().ato.status == AtoStatus::kNormal);
  assert(!context.events.has(EventType::kAtoStart));
}

ResilienceResult runAtoStuckScenario(ResilienceReport& report) {
  AtoStuckContext context;
  const auto* scenario = findResilienceScenario(ResilienceScenarioId::kAtoStuck);
  assert(scenario != nullptr);
  context.setupReady();
  assert(report.beginScenario(*scenario, context.timeSource.uptimeMillis(),
                              currentSystemState()));

  startRefill(context);
  setConnectivityOffline();
  assert(!currentSystemState().network.wifiConnected);
  assert(!currentSystemState().network.mqttConnected);
  assertTimeoutAndCooldown(context);
  recoverAto(context);

  const ResilienceResult result =
      currentSystemState().ato.status == AtoStatus::kNormal &&
              !currentSystemState().relays.atoPump.enabled &&
              context.events.has(EventType::kAtoTimeout) &&
              context.events.has(EventType::kAtoRecovered)
          ? ResilienceResult::kPassed
          : ResilienceResult::kFailed;
  assert(report.completeScenario(scenario->id, result,
                                 context.timeSource.uptimeMillis(),
                                 currentSystemState()));
  return result;
}

void testAtoStuckResilience() {
  ResilienceReport report;
  assert(runAtoStuckScenario(report) == ResilienceResult::kPassed);

  assertModeBlocksAto(FEEDING);
  assertModeBlocksAto(TPA);
  assertModeBlocksAto(MAINTENANCE);

  const auto summary = report.summary();
  assert(summary.scenarioCount == 1);
  assert(summary.passedCount == 1);
  assert(summary.failedCount == 0);
  assert(summary.notRunCount == 0);
}

}  // namespace

int main() {
  testAtoStuckResilience();
  return 0;
}

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
#include "fakes/fake_resilience_mqtt.h"
#include "fakes/fake_resilience_network.h"
#include "fakes/fake_temperature_sensor.h"
#include "fakes/fake_time_source.h"
#include "fakes/fake_watchdog_backend.h"
#include "fakes/fake_water_level_sensor.h"
#include "modules/modes/mode_types.h"
#include "mqtt/mqtt_service.h"
#include "network/network_heartbeat.h"
#include "network/wifi_service.h"
#include "resilience/resilience_report.h"
#include "resilience/resilience_scenarios.h"

namespace {

using reeflow::app::CoreApp;
using reeflow::config::ConfigManager;
using reeflow::core::events::Event;
using reeflow::core::events::EventBus;
using reeflow::core::events::EventType;
using reeflow::core::platform::CorePlatform;
using reeflow::core::state::OperationalMode;
using reeflow::core::state::currentSystemState;
using reeflow::modules::modes::FEEDING;
using reeflow::mqtt::MqttService;
using reeflow::network::NetworkHeartbeat;
using reeflow::network::WifiService;
using reeflow::resilience::ResilienceReport;
using reeflow::resilience::ResilienceResult;
using reeflow::resilience::ResilienceScenarioId;
using reeflow::resilience::findResilienceScenario;
using reeflow::test::fakes::FakeLightingPwmController;
using reeflow::test::fakes::FakeLogSink;
using reeflow::test::fakes::FakeModeStore;
using reeflow::test::fakes::FakeRelayController;
using reeflow::test::fakes::FakeResilienceMqtt;
using reeflow::test::fakes::FakeResilienceNetwork;
using reeflow::test::fakes::FakeTemperatureSensor;
using reeflow::test::fakes::FakeTimeSource;
using reeflow::test::fakes::FakeWatchdogBackend;
using reeflow::test::fakes::FakeWaterLevelSensor;

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

struct NetworkMqttContext {
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
  FakeModeStore modeStore;
  FakeResilienceNetwork network;
  FakeResilienceMqtt mqtt;
  WifiService wifiService;
  NetworkHeartbeat networkHeartbeat;
  MqttService mqttService;
  CoreApp app;

  NetworkMqttContext()
      : platform(timeSource, logSink, watchdogBackend),
        configManager(eventBus),
        wifiService(network.wifi(), configManager, eventBus),
        networkHeartbeat(eventBus),
        mqttService(mqtt.mqtt(), configManager, eventBus),
        app(platform, eventBus, configManager, temperatureSensor,
            waterLevelSensor, relayController, lightingController, modeStore,
            nullptr, &wifiService, nullptr, nullptr, &networkHeartbeat,
            &mqttService) {
    eventBus.subscribe(EventType::kWifiConnected, EventRecorder::capture,
                       &events);
    eventBus.subscribe(EventType::kWifiDisconnected, EventRecorder::capture,
                       &events);
    eventBus.subscribe(EventType::kMqttDisconnected, EventRecorder::capture,
                       &events);
    eventBus.subscribe(EventType::kMqttReconnected, EventRecorder::capture,
                       &events);
  }

  void configureConnectivity() {
    reeflow::config::WifiConfig wifi = {};
    wifi.enabled = true;
    strncpy(wifi.ssid, "reef-local", sizeof(wifi.ssid) - 1);
    strncpy(wifi.password, "reef-pass", sizeof(wifi.password) - 1);
    assert(configManager.updateWifi(wifi));

    reeflow::config::MqttConfig mqttConfig = {};
    mqttConfig.enabled = true;
    strncpy(mqttConfig.host, "broker.local", sizeof(mqttConfig.host) - 1);
    strncpy(mqttConfig.clientId, "reef-device", sizeof(mqttConfig.clientId) - 1);
    strncpy(mqttConfig.topicPrefix, "reeflow", sizeof(mqttConfig.topicPrefix) - 1);
    mqttConfig.port = 1883;
    mqttConfig.cleanSession = true;
    mqttConfig.keepAliveSeconds = 30;
    mqttConfig.connectTimeoutMillis = 10000;
    mqttConfig.heartbeatIntervalMillis = 30000;
    mqttConfig.maxPayloadBytes = 512;
    mqttConfig.maxOutboxMessages = 8;
    mqttConfig.initialBackoffMillis = 1000;
    mqttConfig.maxBackoffMillis = 60000;
    assert(configManager.updateMqtt(mqttConfig));
  }

  void setupOnline() {
    network.startOnline();
    mqtt.startOnline();
    temperatureSensor.setValidTemperature(26.0F);
    waterLevelSensor.setValidLevel(55);
    assert(app.setup());
    configureConnectivity();
    mqtt.mqtt().setConnected(false);
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

bool localAutomationStillRunning(const NetworkMqttContext& context) {
  const auto& state = currentSystemState();
  assert(context.temperatureSensor.readCount() > 0);
  assert(context.waterLevelSensor.readCount() > 0);
  assert(context.watchdogBackend.feedCalls() > 0);
  assert(!state.ato.pumpRunning);
  assert(!state.relays.recalque.enabled);
  assert(!state.relays.heater.enabled);
  assert(!state.relays.atoPump.enabled);
  assert(!state.relays.reserve.enabled);
  assert(context.lightingController.recordedCallCount() > 0);
  return context.temperatureSensor.readCount() > 0 &&
         context.waterLevelSensor.readCount() > 0 &&
         context.watchdogBackend.feedCalls() > 0 &&
         !state.ato.pumpRunning && !state.relays.recalque.enabled &&
         !state.relays.heater.enabled && !state.relays.atoPump.enabled &&
         !state.relays.reserve.enabled &&
         context.lightingController.recordedCallCount() > 0;
}

ResilienceResult runWifiLossScenario(ResilienceReport& report) {
  NetworkMqttContext context;
  const auto* scenario = findResilienceScenario(ResilienceScenarioId::kWifiLoss);
  assert(scenario != nullptr);
  context.setupOnline();
  assert(report.beginScenario(*scenario, context.timeSource.uptimeMillis(),
                              currentSystemState()));

  context.advanceAndRun(1000);
  assert(currentSystemState().network.wifiConnected);
  assert(currentSystemState().network.mqttConnected);

  context.network.failWifi();
  context.advanceAndRun(5000);
  assert(!currentSystemState().network.wifiConnected);
  assert(!currentSystemState().network.mqttConnected);
  assert(context.events.has(EventType::kWifiDisconnected));
  assert(context.events.has(EventType::kMqttDisconnected));
  assert(activeAlertNamed("WIFI_OFFLINE"));
  assert(activeAlertNamed("MQTT_OFFLINE"));
  assert(context.app.requestMode(FEEDING) ==
         reeflow::modules::modes::ModeServiceResult::kSuccess);
  assert(currentSystemState().modes.currentMode == OperationalMode::kFeeding);
  assert(localAutomationStillRunning(context));

  context.network.recoverWifi();
  context.mqtt.recoverMqtt();
  context.advanceAndRun(1000);
  assert(currentSystemState().network.wifiConnected);
  assert(currentSystemState().network.mqttConnected);
  assert(context.events.has(EventType::kWifiConnected));
  assert(context.events.has(EventType::kMqttReconnected));
  assert(strcmp(currentSystemState().alerts.lastRecovery, "MQTT_RECOVERED") ==
             0 ||
         strcmp(currentSystemState().alerts.lastRecovery, "WIFI_RECOVERED") ==
             0);

  const ResilienceResult result =
      currentSystemState().network.wifiConnected &&
              currentSystemState().network.mqttConnected &&
              localAutomationStillRunning(context)
          ? ResilienceResult::kPassed
          : ResilienceResult::kFailed;
  assert(report.completeScenario(scenario->id, result,
                                 context.timeSource.uptimeMillis(),
                                 currentSystemState()));
  return result;
}

ResilienceResult runMqttLossScenario(ResilienceReport& report) {
  NetworkMqttContext context;
  const auto* scenario = findResilienceScenario(ResilienceScenarioId::kMqttLoss);
  assert(scenario != nullptr);
  context.setupOnline();
  assert(report.beginScenario(*scenario, context.timeSource.uptimeMillis(),
                              currentSystemState()));

  context.advanceAndRun(1000);
  assert(currentSystemState().network.wifiConnected);
  assert(currentSystemState().network.mqttConnected);

  context.mqtt.failMqtt();
  context.advanceAndRun(5000);
  assert(currentSystemState().network.wifiConnected);
  assert(!currentSystemState().network.mqttConnected);
  assert(activeAlertNamed("MQTT_OFFLINE"));
  assert(localAutomationStillRunning(context));

  context.mqtt.recoverMqtt();
  context.advanceAndRun(2000);
  assert(currentSystemState().network.wifiConnected);
  assert(currentSystemState().network.mqttConnected);
  assert(context.events.has(EventType::kMqttReconnected));
  assert(strcmp(currentSystemState().alerts.lastRecovery, "MQTT_RECOVERED") ==
         0);

  const ResilienceResult result =
      currentSystemState().network.wifiConnected &&
              currentSystemState().network.mqttConnected &&
              localAutomationStillRunning(context)
          ? ResilienceResult::kPassed
          : ResilienceResult::kFailed;
  assert(report.completeScenario(scenario->id, result,
                                 context.timeSource.uptimeMillis(),
                                 currentSystemState()));
  return result;
}

void testNetworkAndMqttResilience() {
  ResilienceReport report;
  assert(runWifiLossScenario(report) == ResilienceResult::kPassed);
  assert(runMqttLossScenario(report) == ResilienceResult::kPassed);

  const auto summary = report.summary();
  assert(summary.scenarioCount == 2);
  assert(summary.passedCount == 2);
  assert(summary.failedCount == 0);
  assert(summary.notRunCount == 0);
}

}  // namespace

int main() {
  testNetworkAndMqttResilience();
  return 0;
}

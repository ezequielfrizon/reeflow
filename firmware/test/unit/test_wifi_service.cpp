#include <assert.h>
#include <string.h>

#include "config/config_manager.h"
#include "core/events/event_bus.h"
#include "core/state/system_state.h"
#include "fakes/fake_wifi_adapter.h"
#include "network/network_config.h"
#include "network/wifi_service.h"

namespace {

using reeflow::config::ConfigManager;
using reeflow::core::events::Event;
using reeflow::core::events::EventBus;
using reeflow::core::events::EventType;
using reeflow::core::state::SystemState;
using reeflow::network::NetworkConfig;
using reeflow::network::WifiConnectResult;
using reeflow::network::WifiService;
using reeflow::network::makeDefaultNetworkConfig;
using reeflow::test::fakes::FakeWifiAdapter;

struct EventRecorder {
  Event events[16];
  uint8_t count;
};

bool recordEvent(const Event& event, void* context) {
  EventRecorder* recorder = static_cast<EventRecorder*>(context);
  recorder->events[recorder->count++] = event;
  return true;
}

struct Fixture {
  FakeWifiAdapter adapter;
  EventBus eventBus;
  ConfigManager configManager;
  NetworkConfig networkConfig;
  WifiService service;
  EventRecorder connectedEvents;
  EventRecorder disconnectedEvents;
  EventRecorder reconnectingEvents;
  EventRecorder reconnectFailedEvents;

  Fixture()
      : configManager(eventBus),
        networkConfig(makeDefaultNetworkConfig()),
        service(adapter, configManager, eventBus, networkConfig),
        connectedEvents({}),
        disconnectedEvents({}),
        reconnectingEvents({}),
        reconnectFailedEvents({}) {
    reeflow::core::state::resetSystemState();
    reeflow::core::state::setSystemStateEventBus(eventBus);
    eventBus.subscribe(EventType::kWifiConnected, recordEvent,
                       &connectedEvents);
    eventBus.subscribe(EventType::kWifiDisconnected, recordEvent,
                       &disconnectedEvents);
    eventBus.subscribe(EventType::kWifiReconnecting, recordEvent,
                       &reconnectingEvents);
    eventBus.subscribe(EventType::kWifiReconnectFailed, recordEvent,
                       &reconnectFailedEvents);
  }
};

void enableWifi(ConfigManager& configManager) {
  auto wifi = configManager.wifi();
  wifi.enabled = true;
  strcpy(wifi.ssid, "reeflow");
  strcpy(wifi.password, "secret");
  assert(configManager.updateWifi(wifi));
}

void assertUnrelatedBlocksMatch(const SystemState& before) {
  const SystemState& after = reeflow::core::state::currentSystemState();

  assert(after.temperature.currentTemperature ==
         before.temperature.currentTemperature);
  assert(after.temperature.status == before.temperature.status);
  assert(after.waterLevel.currentLevel == before.waterLevel.currentLevel);
  assert(after.waterLevel.status == before.waterLevel.status);
  assert(after.lighting.mode == before.lighting.mode);
  assert(after.relays.recalque.enabled == before.relays.recalque.enabled);
  assert(after.relays.heater.enabled == before.relays.heater.enabled);
  assert(after.relays.atoPump.enabled == before.relays.atoPump.enabled);
  assert(after.relays.reserve.enabled == before.relays.reserve.enabled);
  assert(after.modes.currentMode == before.modes.currentMode);
  assert(after.ato.enabled == before.ato.enabled);
  assert(after.ato.status == before.ato.status);
  assert(after.ato.pumpRunning == before.ato.pumpRunning);
  assert(after.alerts.activeAlertCount == before.alerts.activeAlertCount);
  assert(after.systemHealth.uptime == before.systemHealth.uptime);
}

void testMissingCredentialsDoNotStartAggressiveConnection() {
  Fixture fixture;

  assert(fixture.service.begin(0));
  fixture.service.tick(0);
  fixture.service.tick(1000);

  assert(fixture.adapter.beginCallCount() == 1);
  assert(fixture.adapter.connectCallCount() == 0);
  assert(fixture.service.snapshot().lastConnectResult ==
         WifiConnectResult::kCredentialMissing);
  assert(fixture.service.snapshot().nextReconnectAtMillis == 0);
  assert(!reeflow::core::state::currentSystemState().network.wifiConnected);
  assert(fixture.reconnectingEvents.count == 0);
  assert(fixture.reconnectFailedEvents.count == 0);
}

void testSuccessfulConnectionUpdatesNetworkStateAndEvents() {
  Fixture fixture;
  enableWifi(fixture.configManager);
  fixture.adapter.simulateSuccessfulConnection("192.168.1.20", -55);

  fixture.service.tick(100);

  const auto& network = reeflow::core::state::currentSystemState().network;
  assert(network.wifiConnected);
  assert(strcmp(network.ipAddress, "192.168.1.20") == 0);
  assert(network.rssi == -55);
  assert(!network.internetAvailable);
  assert(!network.mqttConnected);
  assert(fixture.adapter.connectCallCount() == 1);
  assert(fixture.reconnectingEvents.count == 1);
  assert(fixture.connectedEvents.count == 1);
}

void testAuthenticationFailureIsRepresentedAndScheduled() {
  Fixture fixture;
  enableWifi(fixture.configManager);
  fixture.adapter.simulateAuthenticationFailure();

  fixture.service.tick(1000);

  assert(fixture.service.snapshot().lastConnectResult ==
         WifiConnectResult::kAuthenticationFailed);
  assert(fixture.service.snapshot().currentBackoffMillis == 1000);
  assert(fixture.service.snapshot().nextReconnectAtMillis == 2000);
  assert(fixture.reconnectFailedEvents.count == 1);
  assert(!reeflow::core::state::currentSystemState().network.wifiConnected);
}

void testAccessPointUnavailableDoesNotReconnectBeforeBackoff() {
  Fixture fixture;
  enableWifi(fixture.configManager);
  fixture.adapter.simulateAccessPointUnavailable();

  fixture.service.tick(0);
  fixture.service.tick(999);

  assert(fixture.adapter.connectCallCount() == 1);
  assert(fixture.service.snapshot().lastConnectResult ==
         WifiConnectResult::kAccessPointUnavailable);

  fixture.service.tick(1000);
  assert(fixture.adapter.connectCallCount() == 2);
}

void testConnectionTimeoutIsRepresentedWithoutBlockingTicks() {
  Fixture fixture;
  enableWifi(fixture.configManager);
  fixture.adapter.simulateConnectionStarted();

  fixture.service.tick(0);
  fixture.service.tick(14999);

  assert(fixture.adapter.connectCallCount() == 1);
  assert(fixture.reconnectFailedEvents.count == 0);

  fixture.service.tick(15000);

  assert(fixture.service.snapshot().lastConnectResult ==
         WifiConnectResult::kTimeout);
  assert(fixture.reconnectFailedEvents.count == 1);
  assert(fixture.service.snapshot().nextReconnectAtMillis == 16000);
}

void testConnectionDropClearsNetworkStateAndReconnectsWithoutTouchingAutomations() {
  Fixture fixture;
  enableWifi(fixture.configManager);
  fixture.adapter.simulateSuccessfulConnection("192.168.1.20", -55);
  fixture.service.tick(0);

  const SystemState beforeDrop = reeflow::core::state::currentSystemState();
  fixture.adapter.simulateConnectionDrop();
  fixture.service.tick(500);

  const auto& network = reeflow::core::state::currentSystemState().network;
  assert(!network.wifiConnected);
  assert(network.ipAddress[0] == '\0');
  assert(network.rssi == 0);
  assert(!network.mqttConnected);
  assert(fixture.disconnectedEvents.count == 1);
  assert(fixture.service.snapshot().nextReconnectAtMillis == 1500);
  assertUnrelatedBlocksMatch(beforeDrop);

  fixture.adapter.simulateSuccessfulConnection("192.168.1.21", -50);
  fixture.service.tick(1500);

  const auto& reconnectedNetwork =
      reeflow::core::state::currentSystemState().network;
  assert(reconnectedNetwork.wifiConnected);
  assert(strcmp(reconnectedNetwork.ipAddress, "192.168.1.21") == 0);
  assert(reconnectedNetwork.rssi == -50);
  assert(!reconnectedNetwork.mqttConnected);
}

}  // namespace

int main() {
  testMissingCredentialsDoNotStartAggressiveConnection();
  testSuccessfulConnectionUpdatesNetworkStateAndEvents();
  testAuthenticationFailureIsRepresentedAndScheduled();
  testAccessPointUnavailableDoesNotReconnectBeforeBackoff();
  testConnectionTimeoutIsRepresentedWithoutBlockingTicks();
  testConnectionDropClearsNetworkStateAndReconnectsWithoutTouchingAutomations();
  return 0;
}

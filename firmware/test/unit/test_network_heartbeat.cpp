#include <assert.h>

#include "core/events/event_bus.h"
#include "core/state/system_state.h"
#include "network/network_config.h"
#include "network/network_heartbeat.h"

namespace {

using reeflow::core::events::Event;
using reeflow::core::events::EventBus;
using reeflow::core::events::EventType;
using reeflow::core::state::SystemState;
using reeflow::network::NetworkConfig;
using reeflow::network::NetworkHeartbeat;
using reeflow::network::makeDefaultNetworkConfig;

struct EventRecorder {
  Event events[8];
  uint8_t count;
};

bool recordEvent(const Event& event, void* context) {
  EventRecorder* recorder = static_cast<EventRecorder*>(context);
  recorder->events[recorder->count++] = event;
  return true;
}

NetworkConfig heartbeatConfig() {
  NetworkConfig config = makeDefaultNetworkConfig();
  config.heartbeatIntervalMillis = 1000;
  return config;
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
  assert(after.network.wifiConnected == before.network.wifiConnected);
  assert(after.network.mqttConnected == before.network.mqttConnected);
  assert(after.network.internetAvailable == before.network.internetAvailable);
}

void testHeartbeatUpdatesSystemStateAndPublishesLocalEvent() {
  reeflow::core::state::resetSystemState();
  EventBus eventBus;
  reeflow::core::state::setSystemStateEventBus(eventBus);
  EventRecorder recorder = {};
  eventBus.subscribe(EventType::kNetworkHeartbeat, recordEvent, &recorder);
  NetworkHeartbeat heartbeat(eventBus, heartbeatConfig());

  heartbeat.tick(100);

  assert(reeflow::core::state::currentSystemState().network.lastHeartbeat ==
         100);
  assert(heartbeat.snapshot().status.lastHeartbeatMillis == 100);
  assert(heartbeat.snapshot().nextHeartbeatAtMillis == 1100);
  assert(recorder.count == 1);
  assert(recorder.events[0].type == EventType::kNetworkHeartbeat);
}

void testHeartbeatIntervalIsConfigurable() {
  reeflow::core::state::resetSystemState();
  EventBus eventBus;
  reeflow::core::state::setSystemStateEventBus(eventBus);
  EventRecorder recorder = {};
  eventBus.subscribe(EventType::kNetworkHeartbeat, recordEvent, &recorder);
  NetworkHeartbeat heartbeat(eventBus, heartbeatConfig());

  heartbeat.tick(100);
  heartbeat.tick(1099);

  assert(reeflow::core::state::currentSystemState().network.lastHeartbeat ==
         100);
  assert(recorder.count == 1);

  heartbeat.tick(1100);
  assert(reeflow::core::state::currentSystemState().network.lastHeartbeat ==
         1100);
  assert(recorder.count == 2);
}

void testHeartbeatDoesNotAlterAutomationOrMqttState() {
  reeflow::core::state::resetSystemState();
  EventBus eventBus;
  reeflow::core::state::setSystemStateEventBus(eventBus);
  NetworkHeartbeat heartbeat(eventBus, heartbeatConfig());

  const SystemState before = reeflow::core::state::currentSystemState();
  heartbeat.tick(100);

  assertUnrelatedBlocksMatch(before);
}

}  // namespace

int main() {
  testHeartbeatUpdatesSystemStateAndPublishesLocalEvent();
  testHeartbeatIntervalIsConfigurable();
  testHeartbeatDoesNotAlterAutomationOrMqttState();
  return 0;
}

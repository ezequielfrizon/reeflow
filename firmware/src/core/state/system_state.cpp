#include "core/state/system_state.h"

#include <string.h>

#include "core/events/event_bus.h"

namespace reeflow::core::state {
namespace {

RelayEntryState defaultRelayEntry() {
  RelayEntryState relay = {};
  relay.enabled = false;
  relay.lastChanged = 0;
  relay.source = RelaySource::kLocal;
  return relay;
}

LightingChannelState defaultLightingChannel() {
  LightingChannelState channel = {};
  channel.currentPWM = 0;
  channel.maxPWM = 0;
  channel.enabled = false;
  return channel;
}

SystemState state = makeDefaultSystemState();
events::EventBus* stateEventBus = &events::defaultEventBus();

bool sameText(const char* left, const char* right, size_t length) {
  return strncmp(left, right, length) == 0;
}

bool sameSystemBlock(const SystemBlock& left, const SystemBlock& right) {
  return left.version == right.version;
}

bool sameTemperatureState(const TemperatureState& left,
                          const TemperatureState& right) {
  return left.currentTemperature == right.currentTemperature &&
         left.minTemperature == right.minTemperature &&
         left.maxTemperature == right.maxTemperature &&
         left.status == right.status && left.lastUpdate == right.lastUpdate;
}

bool sameWaterLevelState(const WaterLevelState& left,
                         const WaterLevelState& right) {
  return left.currentLevel == right.currentLevel &&
         left.minimumLevel == right.minimumLevel &&
         left.maximumLevel == right.maximumLevel && left.status == right.status &&
         left.lastUpdate == right.lastUpdate;
}

bool sameLightingChannelState(const LightingChannelState& left,
                              const LightingChannelState& right) {
  return left.currentPWM == right.currentPWM && left.maxPWM == right.maxPWM &&
         left.enabled == right.enabled;
}

bool sameLightingState(const LightingState& left, const LightingState& right) {
  return left.mode == right.mode &&
         left.sunriseEnabled == right.sunriseEnabled &&
         left.sunsetEnabled == right.sunsetEnabled &&
         left.acclimationEnabled == right.acclimationEnabled &&
         sameText(left.currentProfile, right.currentProfile,
                  kProfileNameMaxLength) &&
         sameLightingChannelState(left.white, right.white) &&
         sameLightingChannelState(left.blue, right.blue) &&
         sameLightingChannelState(left.royalBlue, right.royalBlue) &&
         sameLightingChannelState(left.uv, right.uv);
}

bool sameRelayEntryState(const RelayEntryState& left,
                         const RelayEntryState& right) {
  return left.enabled == right.enabled && left.lastChanged == right.lastChanged &&
         left.source == right.source;
}

bool sameRelaysState(const RelaysState& left, const RelaysState& right) {
  return sameRelayEntryState(left.recalque, right.recalque) &&
         sameRelayEntryState(left.heater, right.heater) &&
         sameRelayEntryState(left.atoPump, right.atoPump) &&
         sameRelayEntryState(left.reserve, right.reserve);
}

bool sameModesState(const ModesState& left, const ModesState& right) {
  return left.currentMode == right.currentMode &&
         left.startedAt == right.startedAt &&
         left.remainingTime == right.remainingTime;
}

bool sameAtoState(const AtoState& left, const AtoState& right) {
  return left.enabled == right.enabled && left.status == right.status &&
         left.pumpRunning == right.pumpRunning &&
         left.lastActivation == right.lastActivation &&
         left.lastCompletion == right.lastCompletion &&
         left.timeoutCounter == right.timeoutCounter;
}

bool sameNetworkState(const NetworkState& left, const NetworkState& right) {
  return left.wifiConnected == right.wifiConnected &&
         left.mqttConnected == right.mqttConnected &&
         left.internetAvailable == right.internetAvailable &&
         sameText(left.ipAddress, right.ipAddress, kIpAddressMaxLength) &&
         left.rssi == right.rssi && left.lastHeartbeat == right.lastHeartbeat;
}

bool sameActiveAlertState(const ActiveAlertState& left,
                          const ActiveAlertState& right) {
  return left.active == right.active &&
         sameText(left.name, right.name, kAlertNameMaxLength) &&
         left.priority == right.priority && left.raisedAt == right.raisedAt;
}

bool sameAlertsState(const AlertsState& left, const AlertsState& right) {
  if (left.activeAlertCount != right.activeAlertCount ||
      !sameText(left.lastAlert, right.lastAlert, kAlertNameMaxLength) ||
      !sameText(left.lastRecovery, right.lastRecovery, kAlertNameMaxLength) ||
      left.priority != right.priority) {
    return false;
  }

  for (size_t index = 0; index < kMaxActiveAlerts; ++index) {
    if (!sameActiveAlertState(left.activeAlerts[index],
                              right.activeAlerts[index])) {
      return false;
    }
  }

  return true;
}

bool sameSystemHealthState(const SystemHealthState& left,
                           const SystemHealthState& right) {
  return left.uptime == right.uptime && left.freeHeap == right.freeHeap &&
         left.cpuUsage == right.cpuUsage && left.lastReboot == right.lastReboot &&
         sameText(left.rebootReason, right.rebootReason,
                  kRebootReasonMaxLength) &&
         left.watchdogTriggered == right.watchdogTriggered;
}

void publishStateChanged(events::StateArea stateArea) {
  events::Event event = {};
  event.type = events::EventType::kSystemStateChanged;
  event.stateArea = stateArea;
  stateEventBus->publish(event);
}

}  // namespace

SystemState makeDefaultSystemState() {
  SystemState systemState = {};

  systemState.system.version = 0;

  systemState.temperature.currentTemperature = 0.0F;
  systemState.temperature.minTemperature = kDefaultMinTemperatureC;
  systemState.temperature.maxTemperature = kDefaultMaxTemperatureC;
  systemState.temperature.status = TemperatureStatus::kSensorOffline;
  systemState.temperature.lastUpdate = 0;

  systemState.waterLevel.currentLevel = 0;
  systemState.waterLevel.minimumLevel = 0;
  systemState.waterLevel.maximumLevel = 0;
  systemState.waterLevel.status = WaterLevelStatus::kSensorOffline;
  systemState.waterLevel.lastUpdate = 0;

  systemState.lighting.mode = LightingMode::kManual;
  systemState.lighting.sunriseEnabled = false;
  systemState.lighting.sunsetEnabled = false;
  systemState.lighting.acclimationEnabled = false;
  systemState.lighting.white = defaultLightingChannel();
  systemState.lighting.blue = defaultLightingChannel();
  systemState.lighting.royalBlue = defaultLightingChannel();
  systemState.lighting.uv = defaultLightingChannel();

  systemState.relays.recalque = defaultRelayEntry();
  systemState.relays.heater = defaultRelayEntry();
  systemState.relays.atoPump = defaultRelayEntry();
  systemState.relays.reserve = defaultRelayEntry();

  systemState.modes.currentMode = OperationalMode::kNormal;
  systemState.modes.startedAt = 0;
  systemState.modes.remainingTime = 0;

  systemState.ato.enabled = false;
  systemState.ato.status = AtoStatus::kDisabled;
  systemState.ato.pumpRunning = false;
  systemState.ato.lastActivation = 0;
  systemState.ato.lastCompletion = 0;
  systemState.ato.timeoutCounter = 0;

  systemState.network.wifiConnected = false;
  systemState.network.mqttConnected = false;
  systemState.network.internetAvailable = false;
  systemState.network.rssi = 0;
  systemState.network.lastHeartbeat = 0;

  systemState.alerts.activeAlertCount = 0;
  systemState.alerts.priority = AlertPriority::kInfo;

  systemState.systemHealth.uptime = 0;
  systemState.systemHealth.freeHeap = 0;
  systemState.systemHealth.cpuUsage = 0;
  systemState.systemHealth.lastReboot = 0;
  systemState.systemHealth.watchdogTriggered = false;

  return systemState;
}

void resetSystemState() {
  state = makeDefaultSystemState();
}

const SystemState& currentSystemState() {
  return state;
}

void setSystemStateEventBus(events::EventBus& eventBus) {
  stateEventBus = &eventBus;
}

void updateSystemBlock(const SystemBlock& system) {
  if (sameSystemBlock(state.system, system)) {
    return;
  }

  state.system = system;
  publishStateChanged(events::StateArea::kSystem);
}

void updateTemperatureState(const TemperatureState& temperature) {
  if (sameTemperatureState(state.temperature, temperature)) {
    return;
  }

  state.temperature = temperature;
  publishStateChanged(events::StateArea::kTemperature);
}

void updateWaterLevelState(const WaterLevelState& waterLevel) {
  if (sameWaterLevelState(state.waterLevel, waterLevel)) {
    return;
  }

  state.waterLevel = waterLevel;
  publishStateChanged(events::StateArea::kWaterLevel);
}

void updateLightingState(const LightingState& lighting) {
  if (sameLightingState(state.lighting, lighting)) {
    return;
  }

  state.lighting = lighting;
  publishStateChanged(events::StateArea::kLighting);
}

void updateRelaysState(const RelaysState& relays) {
  if (sameRelaysState(state.relays, relays)) {
    return;
  }

  state.relays = relays;
  publishStateChanged(events::StateArea::kRelays);
}

void updateModesState(const ModesState& modes) {
  if (sameModesState(state.modes, modes)) {
    return;
  }

  state.modes = modes;
  publishStateChanged(events::StateArea::kModes);
}

void updateAtoState(const AtoState& ato) {
  if (sameAtoState(state.ato, ato)) {
    return;
  }

  state.ato = ato;
  publishStateChanged(events::StateArea::kAto);
}

void updateNetworkState(const NetworkState& network) {
  if (sameNetworkState(state.network, network)) {
    return;
  }

  state.network = network;
  publishStateChanged(events::StateArea::kNetwork);
}

void updateAlertsState(const AlertsState& alerts) {
  if (sameAlertsState(state.alerts, alerts)) {
    return;
  }

  state.alerts = alerts;
  publishStateChanged(events::StateArea::kAlerts);
}

void updateSystemHealthState(const SystemHealthState& systemHealth) {
  if (sameSystemHealthState(state.systemHealth, systemHealth)) {
    return;
  }

  state.systemHealth = systemHealth;
  publishStateChanged(events::StateArea::kSystemHealth);
}

}  // namespace reeflow::core::state

#include <assert.h>
#include <string.h>

#include "core/state/system_state.h"

namespace {

using reeflow::core::state::AlertPriority;
using reeflow::core::state::AtoStatus;
using reeflow::core::state::LightingMode;
using reeflow::core::state::OperationalMode;
using reeflow::core::state::RelaySource;
using reeflow::core::state::SystemState;
using reeflow::core::state::TemperatureStatus;
using reeflow::core::state::WaterLevelStatus;

void copyText(char* target, size_t targetLength, const char* source) {
  strncpy(target, source, targetLength - 1);
  target[targetLength - 1] = '\0';
}

void assertDefaultState(const SystemState& state) {
  assert(state.system.version == 0);

  assert(state.temperature.currentTemperature == 0.0F);
  assert(state.temperature.minTemperature ==
         reeflow::core::state::kDefaultMinTemperatureC);
  assert(state.temperature.maxTemperature ==
         reeflow::core::state::kDefaultMaxTemperatureC);
  assert(state.temperature.status == TemperatureStatus::kSensorOffline);
  assert(state.temperature.lastUpdate == 0);

  assert(state.waterLevel.currentLevel == 0);
  assert(state.waterLevel.minimumLevel == 0);
  assert(state.waterLevel.maximumLevel == 0);
  assert(state.waterLevel.status == WaterLevelStatus::kSensorOffline);
  assert(state.waterLevel.lastUpdate == 0);

  assert(state.lighting.mode == LightingMode::kManual);
  assert(!state.lighting.sunriseEnabled);
  assert(!state.lighting.sunsetEnabled);
  assert(!state.lighting.acclimationEnabled);
  assert(state.lighting.currentProfile[0] == '\0');
  assert(!state.lighting.white.enabled);
  assert(state.lighting.white.currentPWM == 0);
  assert(state.lighting.white.maxPWM == 0);
  assert(!state.lighting.blue.enabled);
  assert(!state.lighting.royalBlue.enabled);
  assert(!state.lighting.uv.enabled);

  assert(!state.relays.recalque.enabled);
  assert(!state.relays.heater.enabled);
  assert(!state.relays.atoPump.enabled);
  assert(!state.relays.reserve.enabled);
  assert(state.relays.recalque.source == RelaySource::kLocal);
  assert(state.relays.heater.source == RelaySource::kLocal);
  assert(state.relays.atoPump.source == RelaySource::kLocal);
  assert(state.relays.reserve.source == RelaySource::kLocal);

  assert(state.modes.currentMode == OperationalMode::kNormal);
  assert(state.modes.startedAt == 0);
  assert(state.modes.remainingTime == 0);

  assert(!state.ato.enabled);
  assert(state.ato.status == AtoStatus::kDisabled);
  assert(!state.ato.pumpRunning);
  assert(state.ato.timeoutCounter == 0);

  assert(!state.network.wifiConnected);
  assert(!state.network.mqttConnected);
  assert(!state.network.internetAvailable);
  assert(state.network.ipAddress[0] == '\0');
  assert(state.network.rssi == 0);
  assert(state.network.lastHeartbeat == 0);

  assert(state.alerts.activeAlertCount == 0);
  for (size_t index = 0; index < reeflow::core::state::kMaxActiveAlerts;
       ++index) {
    assert(!state.alerts.activeAlerts[index].active);
    assert(state.alerts.activeAlerts[index].name[0] == '\0');
    assert(state.alerts.activeAlerts[index].priority == AlertPriority::kInfo);
    assert(state.alerts.activeAlerts[index].raisedAt == 0);
  }
  assert(state.alerts.lastAlert[0] == '\0');
  assert(state.alerts.lastRecovery[0] == '\0');
  assert(state.alerts.priority == AlertPriority::kInfo);

  assert(state.systemHealth.uptime == 0);
  assert(state.systemHealth.freeHeap == 0);
  assert(state.systemHealth.cpuUsage == 0);
  assert(state.systemHealth.lastReboot == 0);
  assert(state.systemHealth.rebootReason[0] == '\0');
  assert(!state.systemHealth.watchdogTriggered);
}

void testDefaultConstruction() {
  const SystemState defaultState =
      reeflow::core::state::makeDefaultSystemState();

  assertDefaultState(defaultState);
}

void testGlobalStateReadAndReset() {
  reeflow::core::state::resetSystemState();

  assertDefaultState(reeflow::core::state::currentSystemState());

  reeflow::core::state::SystemBlock system =
      reeflow::core::state::currentSystemState().system;
  system.version = 7;
  reeflow::core::state::updateSystemBlock(system);

  assert(reeflow::core::state::currentSystemState().system.version == 7);

  reeflow::core::state::resetSystemState();
  assertDefaultState(reeflow::core::state::currentSystemState());
}

void testTemperatureUpdatePreservesOtherBlocks() {
  reeflow::core::state::resetSystemState();

  reeflow::core::state::TemperatureState temperature =
      reeflow::core::state::currentSystemState().temperature;
  temperature.currentTemperature = 26.5F;
  temperature.status = TemperatureStatus::kNormal;
  temperature.lastUpdate = 100;

  reeflow::core::state::updateTemperatureState(temperature);

  const SystemState& state = reeflow::core::state::currentSystemState();
  assert(state.temperature.currentTemperature == 26.5F);
  assert(state.temperature.status == TemperatureStatus::kNormal);
  assert(state.temperature.lastUpdate == 100);
  assert(state.waterLevel.status == WaterLevelStatus::kSensorOffline);
  assert(!state.relays.recalque.enabled);
  assert(!state.network.wifiConnected);
}

void testBlockUpdatesAreIsolated() {
  reeflow::core::state::resetSystemState();

  reeflow::core::state::SystemBlock system =
      reeflow::core::state::currentSystemState().system;
  system.version = 1;
  reeflow::core::state::updateSystemBlock(system);

  reeflow::core::state::WaterLevelState waterLevel =
      reeflow::core::state::currentSystemState().waterLevel;
  waterLevel.currentLevel = 42;
  waterLevel.minimumLevel = 20;
  waterLevel.maximumLevel = 80;
  waterLevel.status = WaterLevelStatus::kNormal;
  waterLevel.lastUpdate = 200;
  reeflow::core::state::updateWaterLevelState(waterLevel);

  reeflow::core::state::LightingState lighting =
      reeflow::core::state::currentSystemState().lighting;
  lighting.mode = LightingMode::kAutomatic;
  lighting.sunriseEnabled = true;
  lighting.sunsetEnabled = true;
  copyText(lighting.currentProfile,
           reeflow::core::state::kProfileNameMaxLength, "reef-day");
  lighting.white.enabled = true;
  lighting.white.currentPWM = 80;
  lighting.white.maxPWM = 180;
  reeflow::core::state::updateLightingState(lighting);

  reeflow::core::state::RelaysState relays =
      reeflow::core::state::currentSystemState().relays;
  relays.heater.enabled = true;
  relays.heater.lastChanged = 300;
  relays.heater.source = RelaySource::kAutomation;
  reeflow::core::state::updateRelaysState(relays);

  reeflow::core::state::ModesState modes =
      reeflow::core::state::currentSystemState().modes;
  modes.currentMode = OperationalMode::kFeeding;
  modes.startedAt = 400;
  modes.remainingTime = 600;
  reeflow::core::state::updateModesState(modes);

  reeflow::core::state::AtoState ato =
      reeflow::core::state::currentSystemState().ato;
  ato.enabled = true;
  ato.status = AtoStatus::kNormal;
  ato.timeoutCounter = 2;
  reeflow::core::state::updateAtoState(ato);

  reeflow::core::state::NetworkState network =
      reeflow::core::state::currentSystemState().network;
  network.wifiConnected = true;
  network.mqttConnected = false;
  network.internetAvailable = true;
  copyText(network.ipAddress, reeflow::core::state::kIpAddressMaxLength,
           "192.168.1.20");
  network.rssi = -55;
  network.lastHeartbeat = 500;
  reeflow::core::state::updateNetworkState(network);

  reeflow::core::state::AlertsState alerts =
      reeflow::core::state::currentSystemState().alerts;
  alerts.activeAlerts[0].active = true;
  alerts.activeAlerts[0].priority = AlertPriority::kCritical;
  alerts.activeAlerts[0].raisedAt = 900;
  copyText(alerts.activeAlerts[0].name,
           reeflow::core::state::kAlertNameMaxLength, "TEMPERATURE_HIGH");
  alerts.activeAlertCount = 1;
  alerts.priority = AlertPriority::kCritical;
  copyText(alerts.lastAlert, reeflow::core::state::kAlertNameMaxLength,
           "TEMPERATURE_HIGH");
  copyText(alerts.lastRecovery, reeflow::core::state::kAlertNameMaxLength,
           "TEMPERATURE_RECOVERED");
  reeflow::core::state::updateAlertsState(alerts);

  reeflow::core::state::SystemHealthState systemHealth =
      reeflow::core::state::currentSystemState().systemHealth;
  systemHealth.uptime = 700;
  systemHealth.freeHeap = 120000;
  systemHealth.cpuUsage = 10;
  systemHealth.lastReboot = 50;
  copyText(systemHealth.rebootReason,
           reeflow::core::state::kRebootReasonMaxLength, "POWER_ON");
  systemHealth.watchdogTriggered = false;
  reeflow::core::state::updateSystemHealthState(systemHealth);

  const SystemState& state = reeflow::core::state::currentSystemState();
  assert(state.system.version == 1);
  assert(state.waterLevel.currentLevel == 42);
  assert(state.lighting.mode == LightingMode::kAutomatic);
  assert(strcmp(state.lighting.currentProfile, "reef-day") == 0);
  assert(state.relays.heater.enabled);
  assert(!state.relays.recalque.enabled);
  assert(state.modes.currentMode == OperationalMode::kFeeding);
  assert(state.ato.enabled);
  assert(state.network.wifiConnected);
  assert(strcmp(state.network.ipAddress, "192.168.1.20") == 0);
  assert(state.alerts.activeAlertCount == 1);
  assert(state.alerts.activeAlerts[0].active);
  assert(state.alerts.priority == AlertPriority::kCritical);
  assert(strcmp(state.alerts.activeAlerts[0].name, "TEMPERATURE_HIGH") == 0);
  assert(strcmp(state.alerts.lastAlert, "TEMPERATURE_HIGH") == 0);
  assert(state.systemHealth.freeHeap == 120000);
  assert(strcmp(state.systemHealth.rebootReason, "POWER_ON") == 0);
  assert(state.temperature.status == TemperatureStatus::kSensorOffline);
}

}  // namespace

int main() {
  testDefaultConstruction();
  testGlobalStateReadAndReset();
  testTemperatureUpdatePreservesOtherBlocks();
  testBlockUpdatesAreIsolated();
  return 0;
}

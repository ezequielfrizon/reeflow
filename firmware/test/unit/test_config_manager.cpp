#include <assert.h>
#include <string.h>

#include "config/config_manager.h"

namespace {

using reeflow::config::AtoConfig;
using reeflow::config::CalibrationsConfig;
using reeflow::config::ConfigManager;
using reeflow::config::LightingConfig;
using reeflow::config::ModeConfig;
using reeflow::config::MqttConfig;
using reeflow::config::ReeflowConfig;
using reeflow::config::TemperatureConfig;
using reeflow::config::TimersConfig;
using reeflow::config::WifiConfig;
using reeflow::core::events::ConfigDomain;
using reeflow::core::events::Event;
using reeflow::core::events::EventBus;
using reeflow::core::events::EventType;
using reeflow::core::state::LightingMode;
using reeflow::core::state::OperationalMode;

struct EventRecorder {
  Event events[8];
  uint8_t count;
};

bool recordEvent(const Event& event, void* context) {
  EventRecorder* recorder = static_cast<EventRecorder*>(context);
  recorder->events[recorder->count] = event;
  recorder->count += 1;
  return true;
}

void copyText(char* target, size_t targetLength, const char* source) {
  strncpy(target, source, targetLength - 1);
  target[targetLength - 1] = '\0';
}

void assertDefaultConfig(const ReeflowConfig& config) {
  assert(config.temperature.targetTemperature == 26.5F);
  assert(config.temperature.minTemperature == 25.0F);
  assert(config.temperature.maxTemperature == 28.0F);
  assert(config.temperature.readIntervalMillis == 5000);
  assert(config.temperature.sensorOfflineTimeoutMillis == 30000);

  assert(!config.ato.enabled);
  assert(config.ato.minimumLevel == 20);
  assert(config.ato.maximumLevel == 80);
  assert(config.ato.timeoutMillis == 60000);
  assert(config.ato.cooldownMillis == 300000);

  assert(config.lighting.mode == LightingMode::kManual);
  assert(!config.lighting.sunriseEnabled);
  assert(!config.lighting.sunsetEnabled);
  assert(!config.lighting.acclimationEnabled);
  assert(config.lighting.startMinuteOfDay == 0);
  assert(config.lighting.endMinuteOfDay == 0);
  assert(config.lighting.maxIntensityPercent == 100);
  assert(config.lighting.white.maxPWM == 0);

  assert(config.timers.feedingDurationSeconds == 600);
  assert(config.timers.tpaDurationSeconds == 0);
  assert(config.timers.maintenanceDurationSeconds == 0);

  assert(config.mode.currentMode == OperationalMode::kNormal);

  assert(!config.wifi.enabled);
  assert(config.wifi.ssid[0] == '\0');
  assert(config.wifi.password[0] == '\0');

  assert(!config.mqtt.enabled);
  assert(config.mqtt.host[0] == '\0');
  assert(config.mqtt.port == 1883);
  assert(config.mqtt.clientId[0] == '\0');
  assert(config.mqtt.heartbeatIntervalMillis == 30000);

  assert(config.calibrations.temperatureOffset == 0.0F);
  assert(config.calibrations.waterLevelOffset == 0);
}

void testDefaultsAndDomainReads() {
  EventBus bus;
  ConfigManager manager(bus);

  assertDefaultConfig(manager.currentConfig());
  assert(manager.temperature().targetTemperature == 26.5F);
  assert(manager.ato().minimumLevel == 20);
  assert(manager.lighting().maxIntensityPercent == 100);
  assert(manager.timers().feedingDurationSeconds == 600);
  assert(manager.mode().currentMode == OperationalMode::kNormal);
  assert(!manager.wifi().enabled);
  assert(manager.mqtt().port == 1883);
  assert(manager.calibrations().waterLevelOffset == 0);
}

void testValidUpdatesForAllDomains() {
  EventBus bus;
  ConfigManager manager(bus);

  TemperatureConfig temperature = manager.temperature();
  temperature.targetTemperature = 26.0F;
  assert(manager.updateTemperature(temperature));

  AtoConfig ato = manager.ato();
  ato.enabled = true;
  ato.minimumLevel = 25;
  ato.maximumLevel = 75;
  assert(manager.updateAto(ato));

  LightingConfig lighting = manager.lighting();
  lighting.mode = LightingMode::kAutomatic;
  lighting.sunriseEnabled = true;
  lighting.startMinuteOfDay = 480;
  lighting.endMinuteOfDay = 1200;
  lighting.white.enabled = true;
  lighting.white.maxPWM = 200;
  assert(manager.updateLighting(lighting));

  TimersConfig timers = manager.timers();
  timers.feedingDurationSeconds = 900;
  assert(manager.updateTimers(timers));

  ModeConfig mode = manager.mode();
  mode.currentMode = OperationalMode::kFeeding;
  assert(manager.updateMode(mode));

  WifiConfig wifi = manager.wifi();
  wifi.enabled = true;
  copyText(wifi.ssid, sizeof(wifi.ssid), "reef-wifi");
  copyText(wifi.password, sizeof(wifi.password), "reef-pass");
  assert(manager.updateWifi(wifi));

  MqttConfig mqtt = manager.mqtt();
  mqtt.enabled = true;
  copyText(mqtt.host, sizeof(mqtt.host), "broker.local");
  copyText(mqtt.clientId, sizeof(mqtt.clientId), "reeflow-esp32");
  assert(manager.updateMqtt(mqtt));

  CalibrationsConfig calibrations = manager.calibrations();
  calibrations.temperatureOffset = 0.2F;
  calibrations.waterLevelOffset = -3;
  assert(manager.updateCalibrations(calibrations));

  assert(manager.temperature().targetTemperature == 26.0F);
  assert(manager.ato().enabled);
  assert(manager.lighting().mode == LightingMode::kAutomatic);
  assert(manager.timers().feedingDurationSeconds == 900);
  assert(manager.mode().currentMode == OperationalMode::kFeeding);
  assert(strcmp(manager.wifi().ssid, "reef-wifi") == 0);
  assert(strcmp(manager.mqtt().host, "broker.local") == 0);
  assert(manager.calibrations().waterLevelOffset == -3);
}

void testInvalidUpdateIsRejectedAndPreservesPreviousConfig() {
  EventBus bus;
  ConfigManager manager(bus);

  TemperatureConfig validTemperature = manager.temperature();
  validTemperature.targetTemperature = 26.0F;
  assert(manager.updateTemperature(validTemperature));

  TemperatureConfig invalidTemperature = manager.temperature();
  invalidTemperature.minTemperature = 29.0F;
  invalidTemperature.maxTemperature = 28.0F;
  assert(!manager.updateTemperature(invalidTemperature));
  assert(manager.temperature().targetTemperature == 26.0F);
  assert(manager.temperature().minTemperature == 25.0F);

  AtoConfig invalidAto = manager.ato();
  invalidAto.minimumLevel = 80;
  invalidAto.maximumLevel = 20;
  assert(!manager.updateAto(invalidAto));
  assert(manager.ato().minimumLevel == 20);
  assert(manager.ato().maximumLevel == 80);

  MqttConfig invalidMqtt = manager.mqtt();
  invalidMqtt.enabled = true;
  invalidMqtt.port = 1883;
  assert(!manager.updateMqtt(invalidMqtt));
  assert(!manager.mqtt().enabled);
}

void testAcceptedUpdateEmitsConfigChangedEvent() {
  EventBus bus;
  ConfigManager manager(bus);
  EventRecorder recorder = {};

  bus.subscribe(EventType::kConfigChanged, recordEvent, &recorder);

  TemperatureConfig temperature = manager.temperature();
  temperature.targetTemperature = 26.0F;
  assert(manager.updateTemperature(temperature));

  assert(recorder.count == 1);
  assert(recorder.events[0].type == EventType::kConfigChanged);
  assert(recorder.events[0].configDomain == ConfigDomain::kTemperature);

  assert(manager.updateTemperature(temperature));
  assert(recorder.count == 1);
}

void testLoadDefaultsResetsInMemoryConfigWithoutPersistence() {
  EventBus bus;
  ConfigManager manager(bus);

  TimersConfig timers = manager.timers();
  timers.feedingDurationSeconds = 1200;
  assert(manager.updateTimers(timers));
  assert(manager.timers().feedingDurationSeconds == 1200);

  manager.loadDefaults();
  assertDefaultConfig(manager.currentConfig());
}

}  // namespace

int main() {
  testDefaultsAndDomainReads();
  testValidUpdatesForAllDomains();
  testInvalidUpdateIsRejectedAndPreservesPreviousConfig();
  testAcceptedUpdateEmitsConfigChangedEvent();
  testLoadDefaultsResetsInMemoryConfigWithoutPersistence();
  return 0;
}

#include <assert.h>
#include <string.h>

#include "config/config_manager.h"
#include "core/events/event_bus.h"
#include "core/state/system_state.h"
#include "fakes/fake_lighting_pwm_controller.h"
#include "fakes/fake_storage_backend.h"
#include "modules/lighting/lighting_profile.h"
#include "modules/modes/mode_store.h"
#include "storage/storage_schema.h"
#include "storage/storage_service.h"

namespace {

using reeflow::config::ConfigManager;
using reeflow::core::events::ConfigDomain;
using reeflow::core::events::Event;
using reeflow::core::events::EventBus;
using reeflow::core::events::EventType;
using reeflow::storage::StorageResult;
using reeflow::storage::StorageService;
using reeflow::storage::StorageModeStore;
using reeflow::storage::LightingPersistedState;
using reeflow::storage::kStorageSchemaVersion;
using reeflow::storage::storageKeyForDomain;
using reeflow::storage::storageNamespaceForDomain;
using reeflow::modules::lighting::LightingCurveTargetType;
using reeflow::modules::lighting::LightingProfile;
using reeflow::modules::lighting::makeDefaultLightingProfile;
using reeflow::modules::modes::FEEDING;
using reeflow::modules::modes::MAINTENANCE;
using reeflow::modules::modes::NORMAL;
using reeflow::modules::modes::TPA;
using reeflow::modules::modes::ModeStoreLoadStatus;
using reeflow::modules::modes::ModeStoreSaveResult;
using reeflow::modules::modes::OperationalMode;
using reeflow::test::fakes::FakeStorageBackend;
using reeflow::test::fakes::FakeLightingPwmController;

struct Recorder {
  Event events[8];
  uint8_t count;
};

bool recordEvent(const Event& event, void* context) {
  Recorder* recorder = static_cast<Recorder*>(context);
  recorder->events[recorder->count] = event;
  recorder->count += 1;
  return true;
}

ConfigManager makeConfigManager(EventBus& bus) {
  ConfigManager configManager(bus);
  configManager.loadDefaults();
  return configManager;
}

LightingPersistedState makeLightingState(const char* activeName = "reef-day") {
  LightingPersistedState lighting = {};
  lighting.config.mode = reeflow::core::state::LightingMode::kAutomatic;
  lighting.config.sunriseEnabled = true;
  lighting.config.sunsetEnabled = true;
  lighting.config.acclimationEnabled = true;
  lighting.config.startMinuteOfDay = 480;
  lighting.config.endMinuteOfDay = 1200;
  lighting.config.maxIntensityPercent = 90;
  lighting.config.acclimationDays = 10;
  lighting.config.white = {true, 200};
  lighting.config.blue = {true, 210};
  lighting.config.royalBlue = {true, 220};
  lighting.config.uv = {true, 120};

  lighting.profileCount = 2;
  lighting.currentProfileIndex = 0;
  lighting.profiles[0] = makeDefaultLightingProfile();
  strncpy(lighting.profiles[0].name, activeName,
          sizeof(lighting.profiles[0].name) - 1);
  lighting.profiles[0].name[sizeof(lighting.profiles[0].name) - 1] = '\0';
  lighting.profiles[0].maxGlobalIntensityPercent = 80;
  lighting.profiles[0].white.maxDuty = 180;
  lighting.profiles[0].blue.maxDuty = 190;
  lighting.profiles[0].royalBlue.maxDuty = 200;
  lighting.profiles[0].uv.maxDuty = 100;
  lighting.profiles[0].uv.curve.pointCount = 2;
  lighting.profiles[0].uv.curve.points[0] = {
      600, LightingCurveTargetType::kDuty, 0};
  lighting.profiles[0].uv.curve.points[1] = {
      900, LightingCurveTargetType::kIntensityPercent, 50};

  lighting.profiles[1] = makeDefaultLightingProfile();
  strncpy(lighting.profiles[1].name, "reef-night",
          sizeof(lighting.profiles[1].name) - 1);
  lighting.profiles[1].name[sizeof(lighting.profiles[1].name) - 1] = '\0';
  return lighting;
}

struct StoredPayloadHeader {
  uint16_t schemaVersion;
  uint16_t domain;
  uint16_t payloadSize;
  uint16_t reserved;
};

struct CurrentModePayload {
  uint8_t mode;
};

void writeRawModePayload(FakeStorageBackend& backend, uint16_t schemaVersion,
                         CurrentModePayload payload) {
  uint8_t rawPayload[sizeof(StoredPayloadHeader) +
                     sizeof(CurrentModePayload)] = {};
  StoredPayloadHeader header = {schemaVersion,
                                static_cast<uint16_t>(ConfigDomain::kMode),
                                static_cast<uint16_t>(
                                    sizeof(CurrentModePayload)),
                                0};
  memcpy(rawPayload, &header, sizeof(header));
  memcpy(rawPayload + sizeof(header), &payload, sizeof(payload));
  assert(backend.openNamespace(storageNamespaceForDomain(
             ConfigDomain::kMode)) == StorageResult::kSuccess);
  assert(backend.write(storageNamespaceForDomain(ConfigDomain::kMode),
                       storageKeyForDomain(ConfigDomain::kMode),
                       {rawPayload, sizeof(rawPayload)}) ==
         StorageResult::kSuccess);
}

void testWifiSaveAndRestoreDoesNotTouchNetworkRuntime() {
  EventBus bus;
  FakeStorageBackend backend;
  StorageService storage(backend, bus);
  ConfigManager source = makeConfigManager(bus);
  ConfigManager target = makeConfigManager(bus);
  Recorder recorder = {};
  bus.subscribe(EventType::kConfigSaved, recordEvent, &recorder);
  bus.subscribe(EventType::kConfigRestored, recordEvent, &recorder);

  auto wifi = source.wifi();
  wifi.enabled = true;
  strncpy(wifi.ssid, "reef", sizeof(wifi.ssid) - 1);
  strncpy(wifi.password, "secret", sizeof(wifi.password) - 1);
  assert(source.updateWifi(wifi));

  const auto beforeNetwork =
      reeflow::core::state::currentSystemState().network;
  assert(storage.saveWifi(source.wifi(), 10) == StorageResult::kSuccess);
  assert(storage.restoreWifi(target, 20) == StorageResult::kSuccess);

  assert(target.wifi().enabled);
  assert(strcmp(target.wifi().ssid, "reef") == 0);
  assert(strcmp(target.wifi().password, "secret") == 0);
  const auto afterNetwork =
      reeflow::core::state::currentSystemState().network;
  assert(afterNetwork.wifiConnected == beforeNetwork.wifiConnected);
  assert(afterNetwork.mqttConnected == beforeNetwork.mqttConnected);
  assert(recorder.count == 2);
  assert(recorder.events[0].type == EventType::kConfigSaved);
  assert(recorder.events[0].configDomain == ConfigDomain::kWifi);
  assert(recorder.events[1].type == EventType::kConfigRestored);
  assert(recorder.events[1].configDomain == ConfigDomain::kWifi);
}

void testMqttSaveAndRestoreDoesNotTouchNetworkRuntime() {
  EventBus bus;
  FakeStorageBackend backend;
  StorageService storage(backend, bus);
  ConfigManager source = makeConfigManager(bus);
  ConfigManager target = makeConfigManager(bus);

  auto mqtt = source.mqtt();
  mqtt.enabled = true;
  strncpy(mqtt.host, "broker.local", sizeof(mqtt.host) - 1);
  strncpy(mqtt.clientId, "reef-esp32", sizeof(mqtt.clientId) - 1);
  mqtt.port = 1884;
  mqtt.heartbeatIntervalMillis = 45000;
  assert(source.updateMqtt(mqtt));

  const auto beforeNetwork =
      reeflow::core::state::currentSystemState().network;
  assert(storage.saveMqtt(source.mqtt()) == StorageResult::kSuccess);
  assert(storage.restoreMqtt(target) == StorageResult::kSuccess);

  assert(target.mqtt().enabled);
  assert(strcmp(target.mqtt().host, "broker.local") == 0);
  assert(strcmp(target.mqtt().clientId, "reef-esp32") == 0);
  assert(target.mqtt().port == 1884);
  assert(target.mqtt().heartbeatIntervalMillis == 45000);
  const auto afterNetwork =
      reeflow::core::state::currentSystemState().network;
  assert(afterNetwork.wifiConnected == beforeNetwork.wifiConnected);
  assert(afterNetwork.mqttConnected == beforeNetwork.mqttConnected);
}

void testTemperatureLimitsSaveRestoreAndReflectLimitsOnly() {
  EventBus bus;
  FakeStorageBackend backend;
  StorageService storage(backend, bus);
  ConfigManager source = makeConfigManager(bus);
  ConfigManager target = makeConfigManager(bus);
  reeflow::core::state::resetSystemState();

  auto runtimeTemperature =
      reeflow::core::state::currentSystemState().temperature;
  runtimeTemperature.currentTemperature = 26.2F;
  runtimeTemperature.status = reeflow::core::state::TemperatureStatus::kNormal;
  runtimeTemperature.lastUpdate = 99;
  reeflow::core::state::updateTemperatureState(runtimeTemperature);

  auto temperature = source.temperature();
  temperature.minTemperature = 24.5F;
  temperature.maxTemperature = 29.0F;
  temperature.targetTemperature = 26.5F;
  assert(source.updateTemperature(temperature));

  assert(storage.saveTemperature(source.temperature()) ==
         StorageResult::kSuccess);
  assert(storage.restoreTemperature(target) == StorageResult::kSuccess);

  assert(target.temperature().minTemperature == 24.5F);
  assert(target.temperature().maxTemperature == 29.0F);
  const auto restoredRuntime =
      reeflow::core::state::currentSystemState().temperature;
  assert(restoredRuntime.minTemperature == 24.5F);
  assert(restoredRuntime.maxTemperature == 29.0F);
  assert(restoredRuntime.currentTemperature == 26.2F);
  assert(restoredRuntime.status ==
         reeflow::core::state::TemperatureStatus::kNormal);
  assert(restoredRuntime.lastUpdate == 99);
}

void testAtoTimersAndCalibrationsSaveRestore() {
  EventBus bus;
  FakeStorageBackend backend;
  StorageService storage(backend, bus);
  ConfigManager source = makeConfigManager(bus);
  ConfigManager target = makeConfigManager(bus);

  auto ato = source.ato();
  ato.enabled = true;
  ato.minimumLevel = 25;
  ato.maximumLevel = 75;
  ato.timeoutMillis = 120000;
  ato.cooldownMillis = 600000;
  assert(source.updateAto(ato));

  auto timers = source.timers();
  timers.feedingDurationSeconds = 321;
  timers.tpaDurationSeconds = 1000;
  timers.maintenanceDurationSeconds = 2000;
  assert(source.updateTimers(timers));

  auto calibrations = source.calibrations();
  calibrations.temperatureOffset = 1.5F;
  calibrations.waterLevelOffset = -12;
  assert(source.updateCalibrations(calibrations));

  assert(storage.saveAto(source.ato()) == StorageResult::kSuccess);
  assert(storage.saveTimers(source.timers()) == StorageResult::kSuccess);
  assert(storage.saveCalibrations(source.calibrations()) ==
         StorageResult::kSuccess);

  assert(storage.restoreAto(target) == StorageResult::kSuccess);
  assert(storage.restoreTimers(target) == StorageResult::kSuccess);
  assert(storage.restoreCalibrations(target) == StorageResult::kSuccess);

  assert(target.ato().enabled);
  assert(target.ato().minimumLevel == 25);
  assert(target.ato().maximumLevel == 75);
  assert(target.ato().timeoutMillis == 120000);
  assert(target.ato().cooldownMillis == 600000);
  assert(target.timers().feedingDurationSeconds == 321);
  assert(target.timers().tpaDurationSeconds == 1000);
  assert(target.timers().maintenanceDurationSeconds == 2000);
  assert(target.calibrations().temperatureOffset == 1.5F);
  assert(target.calibrations().waterLevelOffset == -12);
  assert(!reeflow::core::state::currentSystemState().ato.pumpRunning);
}

void testMissingDomainKeepsConfigDefaults() {
  EventBus bus;
  FakeStorageBackend backend;
  StorageService storage(backend, bus);
  ConfigManager target = makeConfigManager(bus);

  assert(storage.restoreWifi(target) == StorageResult::kNamespaceNotFound);
  assert(!target.wifi().enabled);
  assert(target.wifi().ssid[0] == '\0');
}

void testCorruptedPayloadDoesNotCorruptCurrentConfig() {
  EventBus bus;
  FakeStorageBackend backend;
  StorageService storage(backend, bus);
  ConfigManager source = makeConfigManager(bus);
  ConfigManager target = makeConfigManager(bus);

  auto timers = source.timers();
  timers.feedingDurationSeconds = 444;
  assert(source.updateTimers(timers));
  assert(storage.saveTimers(source.timers()) == StorageResult::kSuccess);
  backend.simulateCorruptedPayload(
      storageNamespaceForDomain(ConfigDomain::kTimers),
      storageKeyForDomain(ConfigDomain::kTimers));

  const uint32_t defaultFeeding = target.timers().feedingDurationSeconds;
  assert(storage.restoreTimers(target) == StorageResult::kInvalidPayload);
  assert(target.timers().feedingDurationSeconds == defaultFeeding);
}

void testInvalidPayloadRejectedByConfigValidation() {
  EventBus bus;
  FakeStorageBackend backend;
  StorageService storage(backend, bus);
  ConfigManager source = makeConfigManager(bus);
  ConfigManager target = makeConfigManager(bus);

  auto ato = source.ato();
  ato.minimumLevel = 10;
  ato.maximumLevel = 90;
  assert(source.updateAto(ato));
  assert(storage.saveAto(source.ato()) == StorageResult::kSuccess);

  const uint8_t invalidAtoPayload[] = {
      1, 0, 3, 0, 16, 0, 0, 0, 1, 0, 90, 0, 10, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0};
  assert(backend.write(storageNamespaceForDomain(ConfigDomain::kAto),
                       storageKeyForDomain(ConfigDomain::kAto),
                       {invalidAtoPayload, sizeof(invalidAtoPayload)}) ==
         StorageResult::kSuccess);

  assert(storage.restoreAto(target) == StorageResult::kInvalidPayload);
  assert(target.ato().minimumLevel < target.ato().maximumLevel);
}

void testModeSaveAndRestoreAllCanonicalModes() {
  EventBus bus;
  FakeStorageBackend backend;
  StorageService storage(backend, bus);
  Recorder recorder = {};
  bus.subscribe(EventType::kConfigSaved, recordEvent, &recorder);
  bus.subscribe(EventType::kConfigRestored, recordEvent, &recorder);

  const OperationalMode modes[] = {NORMAL, FEEDING, TPA, MAINTENANCE};
  for (OperationalMode mode : modes) {
    OperationalMode restoredMode = NORMAL;
    assert(storage.saveMode(mode, 50) == StorageResult::kSuccess);
    assert(storage.restoreMode(restoredMode, 60) ==
           StorageResult::kSuccess);
    assert(restoredMode == mode);
  }

  assert(recorder.count == 8);
  assert(recorder.events[0].type == EventType::kConfigSaved);
  assert(recorder.events[0].configDomain == ConfigDomain::kMode);
  assert(recorder.events[1].type == EventType::kConfigRestored);
  assert(recorder.events[1].configDomain == ConfigDomain::kMode);
}

void testModeRejectsMissingInvalidCorruptedAndIncompatiblePayloads() {
  EventBus bus;
  FakeStorageBackend backend;
  StorageService storage(backend, bus);
  OperationalMode mode = TPA;

  assert(storage.restoreMode(mode) == StorageResult::kNamespaceNotFound);
  assert(mode == TPA);

  writeRawModePayload(backend, kStorageSchemaVersion, {255});
  assert(storage.restoreMode(mode) == StorageResult::kInvalidPayload);
  assert(mode == TPA);

  writeRawModePayload(backend, kStorageSchemaVersion + 1,
                      {static_cast<uint8_t>(NORMAL)});
  assert(storage.restoreMode(mode) == StorageResult::kIncompatibleSchema);
  assert(mode == TPA);

  assert(storage.saveMode(NORMAL) == StorageResult::kSuccess);
  backend.simulateCorruptedPayload(
      storageNamespaceForDomain(ConfigDomain::kMode),
      storageKeyForDomain(ConfigDomain::kMode));
  assert(storage.restoreMode(mode) == StorageResult::kInvalidPayload);
  assert(mode == TPA);

  assert(storage.saveMode(static_cast<OperationalMode>(255)) ==
         StorageResult::kInvalidPayload);
}

void testStorageModeStoreMapsStorageResultsToModeBoundary() {
  EventBus bus;
  FakeStorageBackend backend;
  StorageService storage(backend, bus);
  StorageModeStore store(storage);

  assert(store.loadMode().status == ModeStoreLoadStatus::kNotFound);
  assert(store.saveMode(MAINTENANCE) == ModeStoreSaveResult::kSuccess);
  auto loaded = store.loadMode();
  assert(loaded.status == ModeStoreLoadStatus::kLoaded);
  assert(loaded.mode == MAINTENANCE);

  writeRawModePayload(backend, kStorageSchemaVersion, {255});
  loaded = store.loadMode();
  assert(loaded.status == ModeStoreLoadStatus::kInvalidMode);

  backend.simulateWriteFailure();
  assert(store.saveMode(TPA) == ModeStoreSaveResult::kFailure);
}

void testLightingSaveAndRestoreProfileWithoutPwmWrites() {
  EventBus bus;
  FakeStorageBackend backend;
  StorageService storage(backend, bus);
  ConfigManager target = makeConfigManager(bus);
  Recorder recorder = {};
  FakeLightingPwmController pwm;
  reeflow::core::state::resetSystemState();
  auto runtimeLighting = reeflow::core::state::currentSystemState().lighting;
  runtimeLighting.white.currentPWM = 77;
  runtimeLighting.blue.currentPWM = 66;
  runtimeLighting.royalBlue.currentPWM = 55;
  runtimeLighting.uv.currentPWM = 44;
  reeflow::core::state::updateLightingState(runtimeLighting);
  bus.subscribe(EventType::kConfigSaved, recordEvent, &recorder);
  bus.subscribe(EventType::kConfigRestored, recordEvent, &recorder);
  bus.subscribe(EventType::kLightingStarted, recordEvent, &recorder);
  bus.subscribe(EventType::kLightingStopped, recordEvent, &recorder);

  const LightingPersistedState sourceLighting = makeLightingState();
  LightingPersistedState restoredLighting = {};
  assert(storage.saveLighting(sourceLighting, 30) ==
         StorageResult::kSuccess);
  assert(storage.restoreLighting(target, restoredLighting, 40) ==
         StorageResult::kSuccess);

  assert(target.lighting().mode ==
         reeflow::core::state::LightingMode::kAutomatic);
  assert(target.lighting().sunriseEnabled);
  assert(target.lighting().sunsetEnabled);
  assert(target.lighting().acclimationEnabled);
  assert(restoredLighting.profileCount == 2);
  assert(restoredLighting.currentProfileIndex == 0);
  assert(strcmp(restoredLighting.profiles[0].name, "reef-day") == 0);
  assert(restoredLighting.profiles[0].uv.curve.pointCount == 2);

  const auto restoredRuntime =
      reeflow::core::state::currentSystemState().lighting;
  assert(strcmp(restoredRuntime.currentProfile, "reef-day") == 0);
  assert(restoredRuntime.white.currentPWM == 77);
  assert(restoredRuntime.blue.currentPWM == 66);
  assert(restoredRuntime.royalBlue.currentPWM == 55);
  assert(restoredRuntime.uv.currentPWM == 44);
  assert(restoredRuntime.white.maxPWM == 180);
  assert(restoredRuntime.uv.maxPWM == 100);
  assert(pwm.recordedCallCount() == 0);
  assert(recorder.count == 2);
  assert(recorder.events[0].type == EventType::kConfigSaved);
  assert(recorder.events[0].configDomain == ConfigDomain::kLighting);
  assert(recorder.events[1].type == EventType::kConfigRestored);
  assert(recorder.events[1].configDomain == ConfigDomain::kLighting);
}

void testLightingRejectsInvalidProfileAndCurve() {
  EventBus bus;
  FakeStorageBackend backend;
  StorageService storage(backend, bus);
  LightingPersistedState lighting = makeLightingState();

  lighting.profileCount = 0;
  assert(storage.saveLighting(lighting) == StorageResult::kInvalidPayload);

  lighting = makeLightingState();
  lighting.currentProfileIndex = lighting.profileCount;
  assert(storage.saveLighting(lighting) == StorageResult::kInvalidPayload);

  lighting = makeLightingState();
  memset(lighting.profiles[0].name, 'x', sizeof(lighting.profiles[0].name));
  assert(storage.saveLighting(lighting) == StorageResult::kInvalidPayload);

  lighting = makeLightingState();
  lighting.profiles[0].uv.curve.points[1].minuteOfDay =
      lighting.profiles[0].uv.curve.points[0].minuteOfDay;
  assert(storage.saveLighting(lighting) == StorageResult::kInvalidPayload);
}

void testLightingCorruptedPayloadKeepsCurrentConfig() {
  EventBus bus;
  FakeStorageBackend backend;
  StorageService storage(backend, bus);
  ConfigManager target = makeConfigManager(bus);
  LightingPersistedState sourceLighting = makeLightingState();
  LightingPersistedState restoredLighting = {};

  assert(storage.saveLighting(sourceLighting) == StorageResult::kSuccess);
  backend.simulateCorruptedPayload(
      storageNamespaceForDomain(ConfigDomain::kLighting),
      storageKeyForDomain(ConfigDomain::kLighting));

  const auto defaultMode = target.lighting().mode;
  assert(storage.restoreLighting(target, restoredLighting) ==
         StorageResult::kInvalidPayload);
  assert(target.lighting().mode == defaultMode);
}

void testLightingInvalidStoredProfileDoesNotCorruptConfig() {
  EventBus bus;
  FakeStorageBackend backend;
  StorageService storage(backend, bus);
  ConfigManager target = makeConfigManager(bus);
  LightingPersistedState storedLighting = makeLightingState();
  LightingPersistedState restoredLighting = {};
  storedLighting.profiles[0].white.curve.points[1].value = 999;

  uint8_t rawPayload[sizeof(StoredPayloadHeader) +
                     sizeof(LightingPersistedState)] = {};
  StoredPayloadHeader header = {
      kStorageSchemaVersion,
      static_cast<uint16_t>(ConfigDomain::kLighting),
      static_cast<uint16_t>(sizeof(LightingPersistedState)),
      0};
  memcpy(rawPayload, &header, sizeof(header));
  memcpy(rawPayload + sizeof(header), &storedLighting, sizeof(storedLighting));
  assert(backend.openNamespace(storageNamespaceForDomain(
             ConfigDomain::kLighting)) == StorageResult::kSuccess);
  assert(backend.write(storageNamespaceForDomain(ConfigDomain::kLighting),
                       storageKeyForDomain(ConfigDomain::kLighting),
                       {rawPayload, sizeof(rawPayload)}) ==
         StorageResult::kSuccess);

  const auto defaultMode = target.lighting().mode;
  assert(storage.restoreLighting(target, restoredLighting) ==
         StorageResult::kInvalidPayload);
  assert(target.lighting().mode == defaultMode);
}

}  // namespace

int main() {
  testWifiSaveAndRestoreDoesNotTouchNetworkRuntime();
  testMqttSaveAndRestoreDoesNotTouchNetworkRuntime();
  testTemperatureLimitsSaveRestoreAndReflectLimitsOnly();
  testAtoTimersAndCalibrationsSaveRestore();
  testMissingDomainKeepsConfigDefaults();
  testCorruptedPayloadDoesNotCorruptCurrentConfig();
  testInvalidPayloadRejectedByConfigValidation();
  testModeSaveAndRestoreAllCanonicalModes();
  testModeRejectsMissingInvalidCorruptedAndIncompatiblePayloads();
  testStorageModeStoreMapsStorageResultsToModeBoundary();
  testLightingSaveAndRestoreProfileWithoutPwmWrites();
  testLightingRejectsInvalidProfileAndCurve();
  testLightingCorruptedPayloadKeepsCurrentConfig();
  testLightingInvalidStoredProfileDoesNotCorruptConfig();
  return 0;
}

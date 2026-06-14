#include "storage/storage_service.h"

#include <string.h>

#include "core/state/system_state.h"
#include "modules/lighting/lighting_types.h"
#include "storage/storage_events.h"
#include "storage/storage_schema.h"

namespace reeflow::storage {
namespace {

struct StoredPayloadHeader {
  uint16_t schemaVersion;
  uint16_t domain;
  uint16_t payloadSize;
  uint16_t reserved;
};

struct TemperatureLimitsPayload {
  float minTemperature;
  float maxTemperature;
};

struct CurrentModePayload {
  uint8_t mode;
};

const modules::lighting::LightingChannel kLightingChannels[] = {
    modules::lighting::WHITE, modules::lighting::BLUE,
    modules::lighting::ROYAL_BLUE, modules::lighting::UV};

constexpr uint16_t domainValue(StorageDomain domain) {
  return static_cast<uint16_t>(domain);
}

StorageResult validateReadPayload(const StoragePayload& payload,
                                  StorageDomain domain, size_t dataSize) {
  if (payload.data == nullptr ||
      payload.size < sizeof(StoredPayloadHeader)) {
    return StorageResult::kInvalidPayload;
  }

  StoredPayloadHeader header = {};
  memcpy(&header, payload.data, sizeof(header));
  if (!schemaVersionIsSupported(header.schemaVersion)) {
    return StorageResult::kIncompatibleSchema;
  }

  if (header.domain != domainValue(domain) ||
      header.payloadSize != dataSize ||
      payload.size != sizeof(StoredPayloadHeader) + dataSize) {
    return StorageResult::kInvalidPayload;
  }

  return StorageResult::kSuccess;
}

void reflectTemperatureLimits(const config::TemperatureConfig& temperature) {
  core::state::TemperatureState state =
      core::state::currentSystemState().temperature;
  state.minTemperature = temperature.minTemperature;
  state.maxTemperature = temperature.maxTemperature;
  core::state::updateTemperatureState(state);
}

bool validLightingConfig(const config::LightingConfig& config) {
  return modules::lighting::isCanonicalLightingMode(config.mode) &&
         modules::lighting::validLightingMinuteOfDay(
             config.startMinuteOfDay) &&
         modules::lighting::validLightingMinuteOfDay(config.endMinuteOfDay) &&
         modules::lighting::validLightingIntensityPercent(
             config.maxIntensityPercent) &&
         modules::lighting::validLightingDuty(config.white.maxPWM) &&
         modules::lighting::validLightingDuty(config.blue.maxPWM) &&
         modules::lighting::validLightingDuty(config.royalBlue.maxPWM) &&
         modules::lighting::validLightingDuty(config.uv.maxPWM);
}

bool validLightingPersistedState(const LightingPersistedState& lighting) {
  if (!validLightingConfig(lighting.config) || lighting.profileCount == 0 ||
      lighting.profileCount >
          modules::lighting::kLightingMaxProfilesInMemory ||
      lighting.currentProfileIndex >= lighting.profileCount) {
    return false;
  }

  for (uint8_t index = 0; index < lighting.profileCount; ++index) {
    if (modules::lighting::validateLightingProfile(lighting.profiles[index]) !=
        modules::lighting::LightingProfileValidationResult::kValid) {
      return false;
    }
  }

  return true;
}

const config::LightingChannelConfig& configChannel(
    const config::LightingConfig& config,
    modules::lighting::LightingChannel channel) {
  switch (channel) {
    case modules::lighting::LightingChannel::kWhite:
      return config.white;
    case modules::lighting::LightingChannel::kBlue:
      return config.blue;
    case modules::lighting::LightingChannel::kRoyalBlue:
      return config.royalBlue;
    case modules::lighting::LightingChannel::kUv:
      return config.uv;
    case modules::lighting::LightingChannel::kUnknown:
      return config.white;
  }

  return config.white;
}

const modules::lighting::LightingChannelProfile& profileChannel(
    const modules::lighting::LightingProfile& profile,
    modules::lighting::LightingChannel channel) {
  switch (channel) {
    case modules::lighting::LightingChannel::kWhite:
      return profile.white;
    case modules::lighting::LightingChannel::kBlue:
      return profile.blue;
    case modules::lighting::LightingChannel::kRoyalBlue:
      return profile.royalBlue;
    case modules::lighting::LightingChannel::kUv:
      return profile.uv;
    case modules::lighting::LightingChannel::kUnknown:
      return profile.white;
  }

  return profile.white;
}

void reflectLightingConfiguration(const LightingPersistedState& lighting) {
  core::state::LightingState state = core::state::currentSystemState().lighting;
  const modules::lighting::LightingProfile& profile =
      lighting.profiles[lighting.currentProfileIndex];

  state.mode = lighting.config.mode;
  state.sunriseEnabled = lighting.config.sunriseEnabled;
  state.sunsetEnabled = lighting.config.sunsetEnabled;
  state.acclimationEnabled = lighting.config.acclimationEnabled;
  strncpy(state.currentProfile, profile.name,
          core::state::kProfileNameMaxLength - 1);
  state.currentProfile[core::state::kProfileNameMaxLength - 1] = '\0';

  for (const modules::lighting::LightingChannel channel : kLightingChannels) {
    core::state::LightingChannelState* channelState =
        modules::lighting::lightingChannelState(state, channel);
    const config::LightingChannelConfig& channelConfig =
        configChannel(lighting.config, channel);
    const modules::lighting::LightingChannelProfile& channelProfile =
        profileChannel(profile, channel);
    channelState->maxPWM = channelConfig.maxPWM < channelProfile.maxDuty
                               ? channelConfig.maxPWM
                               : channelProfile.maxDuty;
    channelState->enabled = channelConfig.enabled && channelProfile.enabled;
  }

  core::state::updateLightingState(state);
}

bool validModePayload(const CurrentModePayload& payload) {
  const modules::modes::OperationalMode mode =
      static_cast<modules::modes::OperationalMode>(payload.mode);
  return modules::modes::isCanonicalMode(mode);
}

}  // namespace

StorageService::StorageService(StorageBackend& backend)
    : StorageService(backend, core::events::defaultEventBus()) {}

StorageService::StorageService(StorageBackend& backend,
                               core::events::EventBus& eventBus)
    : backend_(backend), eventBus_(eventBus) {}

StorageResult StorageService::saveWifi(const config::WifiConfig& wifi,
                                       uint32_t timestampMillis) {
  return savePayload(StorageDomain::kWifi, &wifi, sizeof(wifi),
                     timestampMillis);
}

StorageResult StorageService::restoreWifi(config::ConfigManager& configManager,
                                          uint32_t timestampMillis) {
  config::WifiConfig wifi = {};
  const StorageResult result =
      readPayload(StorageDomain::kWifi, &wifi, sizeof(wifi));
  if (result != StorageResult::kSuccess) {
    return result;
  }

  if (!configManager.updateWifi(wifi)) {
    return StorageResult::kInvalidPayload;
  }

  publishConfigEvent(StorageEventType::kConfigRestored, StorageDomain::kWifi,
                     StorageResult::kSuccess, timestampMillis,
                     "wifi restored");
  return StorageResult::kSuccess;
}

StorageResult StorageService::saveMqtt(const config::MqttConfig& mqtt,
                                       uint32_t timestampMillis) {
  return savePayload(StorageDomain::kMqtt, &mqtt, sizeof(mqtt),
                     timestampMillis);
}

StorageResult StorageService::restoreMqtt(config::ConfigManager& configManager,
                                          uint32_t timestampMillis) {
  config::MqttConfig mqtt = {};
  const StorageResult result =
      readPayload(StorageDomain::kMqtt, &mqtt, sizeof(mqtt));
  if (result != StorageResult::kSuccess) {
    return result;
  }

  if (!configManager.updateMqtt(mqtt)) {
    return StorageResult::kInvalidPayload;
  }

  publishConfigEvent(StorageEventType::kConfigRestored, StorageDomain::kMqtt,
                     StorageResult::kSuccess, timestampMillis,
                     "mqtt restored");
  return StorageResult::kSuccess;
}

StorageResult StorageService::saveTemperature(
    const config::TemperatureConfig& temperature, uint32_t timestampMillis) {
  const TemperatureLimitsPayload payload = {temperature.minTemperature,
                                            temperature.maxTemperature};
  return savePayload(StorageDomain::kTemperature, &payload, sizeof(payload),
                     timestampMillis);
}

StorageResult StorageService::restoreTemperature(
    config::ConfigManager& configManager, uint32_t timestampMillis) {
  TemperatureLimitsPayload payload = {};
  const StorageResult result = readPayload(StorageDomain::kTemperature,
                                           &payload, sizeof(payload));
  if (result != StorageResult::kSuccess) {
    return result;
  }

  config::TemperatureConfig temperature = configManager.temperature();
  temperature.minTemperature = payload.minTemperature;
  temperature.maxTemperature = payload.maxTemperature;
  if (temperature.targetTemperature < temperature.minTemperature ||
      temperature.targetTemperature > temperature.maxTemperature) {
    temperature.targetTemperature = temperature.minTemperature;
  }

  if (!configManager.updateTemperature(temperature)) {
    return StorageResult::kInvalidPayload;
  }

  reflectTemperatureLimits(temperature);
  publishConfigEvent(StorageEventType::kConfigRestored,
                     StorageDomain::kTemperature, StorageResult::kSuccess,
                     timestampMillis, "temperature restored");
  return StorageResult::kSuccess;
}

StorageResult StorageService::saveAto(const config::AtoConfig& ato,
                                      uint32_t timestampMillis) {
  return savePayload(StorageDomain::kAto, &ato, sizeof(ato),
                     timestampMillis);
}

StorageResult StorageService::restoreAto(config::ConfigManager& configManager,
                                         uint32_t timestampMillis) {
  config::AtoConfig ato = {};
  const StorageResult result =
      readPayload(StorageDomain::kAto, &ato, sizeof(ato));
  if (result != StorageResult::kSuccess) {
    return result;
  }

  if (!configManager.updateAto(ato)) {
    return StorageResult::kInvalidPayload;
  }

  publishConfigEvent(StorageEventType::kConfigRestored, StorageDomain::kAto,
                     StorageResult::kSuccess, timestampMillis,
                     "ato restored");
  return StorageResult::kSuccess;
}

StorageResult StorageService::saveTimers(const config::TimersConfig& timers,
                                         uint32_t timestampMillis) {
  return savePayload(StorageDomain::kTimers, &timers, sizeof(timers),
                     timestampMillis);
}

StorageResult StorageService::restoreTimers(
    config::ConfigManager& configManager, uint32_t timestampMillis) {
  config::TimersConfig timers = {};
  const StorageResult result =
      readPayload(StorageDomain::kTimers, &timers, sizeof(timers));
  if (result != StorageResult::kSuccess) {
    return result;
  }

  if (!configManager.updateTimers(timers)) {
    return StorageResult::kInvalidPayload;
  }

  publishConfigEvent(StorageEventType::kConfigRestored, StorageDomain::kTimers,
                     StorageResult::kSuccess, timestampMillis,
                     "timers restored");
  return StorageResult::kSuccess;
}

StorageResult StorageService::saveCalibrations(
    const config::CalibrationsConfig& calibrations,
    uint32_t timestampMillis) {
  return savePayload(StorageDomain::kCalibrations, &calibrations,
                     sizeof(calibrations), timestampMillis);
}

StorageResult StorageService::restoreCalibrations(
    config::ConfigManager& configManager, uint32_t timestampMillis) {
  config::CalibrationsConfig calibrations = {};
  const StorageResult result = readPayload(StorageDomain::kCalibrations,
                                           &calibrations,
                                           sizeof(calibrations));
  if (result != StorageResult::kSuccess) {
    return result;
  }

  if (!configManager.updateCalibrations(calibrations)) {
    return StorageResult::kInvalidPayload;
  }

  publishConfigEvent(StorageEventType::kConfigRestored,
                     StorageDomain::kCalibrations, StorageResult::kSuccess,
                     timestampMillis, "calibrations restored");
  return StorageResult::kSuccess;
}

StorageResult StorageService::saveLighting(
    const LightingPersistedState& lighting, uint32_t timestampMillis) {
  if (!validLightingPersistedState(lighting)) {
    return StorageResult::kInvalidPayload;
  }

  return savePayload(StorageDomain::kLighting, &lighting, sizeof(lighting),
                     timestampMillis);
}

StorageResult StorageService::restoreLighting(
    config::ConfigManager& configManager, LightingPersistedState& lighting,
    uint32_t timestampMillis) {
  LightingPersistedState restoredLighting = {};
  const StorageResult result =
      readPayload(StorageDomain::kLighting, &restoredLighting,
                  sizeof(restoredLighting));
  if (result != StorageResult::kSuccess) {
    return result;
  }

  if (!validLightingPersistedState(restoredLighting)) {
    return StorageResult::kInvalidPayload;
  }

  if (!configManager.updateLighting(restoredLighting.config)) {
    return StorageResult::kInvalidPayload;
  }

  lighting = restoredLighting;
  reflectLightingConfiguration(restoredLighting);
  publishConfigEvent(StorageEventType::kConfigRestored,
                     StorageDomain::kLighting, StorageResult::kSuccess,
                     timestampMillis, "lighting restored");
  return StorageResult::kSuccess;
}

StorageResult StorageService::saveMode(modules::modes::OperationalMode mode,
                                       uint32_t timestampMillis) {
  if (!modules::modes::isCanonicalMode(mode)) {
    return StorageResult::kInvalidPayload;
  }

  const CurrentModePayload payload = {static_cast<uint8_t>(mode)};
  return savePayload(StorageDomain::kMode, &payload, sizeof(payload),
                     timestampMillis);
}

StorageResult StorageService::restoreMode(
    modules::modes::OperationalMode& mode, uint32_t timestampMillis) {
  CurrentModePayload payload = {};
  const StorageResult result =
      readPayload(StorageDomain::kMode, &payload, sizeof(payload));
  if (result != StorageResult::kSuccess) {
    return result;
  }

  if (!validModePayload(payload)) {
    return StorageResult::kInvalidPayload;
  }

  mode = static_cast<modules::modes::OperationalMode>(payload.mode);
  publishConfigEvent(StorageEventType::kConfigRestored, StorageDomain::kMode,
                     StorageResult::kSuccess, timestampMillis,
                     "mode restored");
  return StorageResult::kSuccess;
}

StorageResult StorageService::markConfigChanged(
    const config::ConfigManager& configManager, StorageDomain domain) {
  switch (domain) {
    case StorageDomain::kWifi:
      return markPayloadChanged(domain, &configManager.wifi(),
                                sizeof(configManager.wifi()));
    case StorageDomain::kMqtt:
      return markPayloadChanged(domain, &configManager.mqtt(),
                                sizeof(configManager.mqtt()));
    case StorageDomain::kTemperature: {
      const TemperatureLimitsPayload payload = {
          configManager.temperature().minTemperature,
          configManager.temperature().maxTemperature};
      return markPayloadChanged(domain, &payload, sizeof(payload));
    }
    case StorageDomain::kAto:
      return markPayloadChanged(domain, &configManager.ato(),
                                sizeof(configManager.ato()));
    case StorageDomain::kTimers:
      return markPayloadChanged(domain, &configManager.timers(),
                                sizeof(configManager.timers()));
    case StorageDomain::kMode: {
      const CurrentModePayload payload = {
          static_cast<uint8_t>(configManager.mode().currentMode)};
      return markPayloadChanged(domain, &payload, sizeof(payload));
    }
    case StorageDomain::kCalibrations:
      return markPayloadChanged(domain, &configManager.calibrations(),
                                sizeof(configManager.calibrations()));
    case StorageDomain::kLighting:
    case StorageDomain::kNone:
      return StorageResult::kIgnoredByWritePolicy;
  }

  return StorageResult::kIgnoredByWritePolicy;
}

StorageResult StorageService::markLightingChanged(
    const LightingPersistedState& lighting) {
  if (!validLightingPersistedState(lighting)) {
    return StorageResult::kInvalidPayload;
  }

  return markPayloadChanged(StorageDomain::kLighting, &lighting,
                            sizeof(lighting));
}

StorageResult StorageService::flushDirty(StorageDomain domain,
                                         uint32_t timestampMillis) {
  const StorageResult result = writePolicy_.flushDomain(backend_, domain);
  if (result == StorageResult::kSuccess) {
    publishConfigEvent(StorageEventType::kConfigSaved, domain, result,
                       timestampMillis, "config flushed");
  }

  return result;
}

StorageResult StorageService::flushAllDirty(uint32_t timestampMillis) {
  StorageResult lastResult = StorageResult::kNoChange;

  const StorageDomain domains[] = {
      StorageDomain::kWifi,         StorageDomain::kMqtt,
      StorageDomain::kTemperature,  StorageDomain::kAto,
      StorageDomain::kTimers,       StorageDomain::kMode,
      StorageDomain::kCalibrations, StorageDomain::kLighting,
  };

  for (StorageDomain domain : domains) {
    if (!writePolicy_.isDirty(domain)) {
      continue;
    }

    lastResult = flushDirty(domain, timestampMillis);
    if (lastResult != StorageResult::kSuccess) {
      return lastResult;
    }
  }

  return lastResult;
}

uint8_t StorageService::dirtyCount() const {
  return writePolicy_.dirtyCount();
}

StorageResult StorageService::markPayloadChanged(StorageDomain domain,
                                                 const void* data,
                                                 size_t dataSize) {
  uint8_t payload[kStorageMaxPayloadSize] = {};
  size_t payloadSize = 0;
  const StorageResult result =
      makeStoredPayload(domain, data, dataSize, payload, payloadSize);
  if (result != StorageResult::kSuccess) {
    return result;
  }

  return writePolicy_.markChanged(
      domain, storageNamespaceForDomain(domain), storageKeyForDomain(domain),
      {payload, payloadSize});
}

StorageResult StorageService::savePayload(StorageDomain domain,
                                          const void* data, size_t dataSize,
                                          uint32_t timestampMillis) {
  const char* namespaceName = storageNamespaceForDomain(domain);
  const char* key = storageKeyForDomain(domain);
  const StorageResult openResult = backend_.openNamespace(namespaceName);
  if (openResult != StorageResult::kSuccess) {
    return openResult;
  }

  uint8_t payload[kStorageMaxPayloadSize] = {};
  size_t payloadSize = 0;
  const StorageResult payloadResult =
      makeStoredPayload(domain, data, dataSize, payload, payloadSize);
  if (payloadResult != StorageResult::kSuccess) {
    return payloadResult;
  }

  const StorageResult result =
      backend_.write(namespaceName, key, {payload, payloadSize});
  if (result == StorageResult::kSuccess) {
    publishConfigEvent(StorageEventType::kConfigSaved, domain, result,
                       timestampMillis, "config saved");
  }

  return result;
}

StorageResult StorageService::makeStoredPayload(StorageDomain domain,
                                                const void* data,
                                                size_t dataSize,
                                                uint8_t* payload,
                                                size_t& payloadSize) const {
  if (data == nullptr || payload == nullptr ||
      dataSize + sizeof(StoredPayloadHeader) > kStorageMaxPayloadSize) {
    return StorageResult::kInvalidPayload;
  }

  const StoredPayloadHeader header = {kStorageSchemaVersion,
                                      domainValue(domain),
                                      static_cast<uint16_t>(dataSize), 0};
  memcpy(payload, &header, sizeof(header));
  memcpy(payload + sizeof(header), data, dataSize);
  payloadSize = sizeof(header) + dataSize;
  return StorageResult::kSuccess;
}

StorageResult StorageService::readPayload(StorageDomain domain, void* data,
                                          size_t dataSize) const {
  if (data == nullptr || dataSize == 0) {
    return StorageResult::kInvalidPayload;
  }

  const StorageReadResult readResult =
      backend_.read(storageNamespaceForDomain(domain),
                    storageKeyForDomain(domain));
  if (readResult.result != StorageResult::kSuccess) {
    return readResult.result;
  }

  const StorageResult payloadResult =
      validateReadPayload(readResult.payload, domain, dataSize);
  if (payloadResult != StorageResult::kSuccess) {
    return payloadResult;
  }

  memcpy(data, readResult.payload.data + sizeof(StoredPayloadHeader),
         dataSize);
  return StorageResult::kSuccess;
}

void StorageService::publishConfigEvent(StorageEventType eventType,
                                        StorageDomain domain,
                                        StorageResult result,
                                        uint32_t timestampMillis,
                                        const char* reason) {
  StorageEventData data = {};
  data.domain = domain;
  data.result = result;
  data.reason = reason;
  data.timestampMillis = timestampMillis;
  eventBus_.publish(makeStorageCoreEvent(eventType, data));
}

StorageModeStore::StorageModeStore(StorageService& storageService)
    : storageService_(storageService) {}

modules::modes::ModeStoreLoadResult StorageModeStore::loadMode() {
  modules::modes::OperationalMode mode = modules::modes::NORMAL;
  const StorageResult result = storageService_.restoreMode(mode);
  switch (result) {
    case StorageResult::kSuccess:
      return modules::modes::makeLoadedModeResult(mode);
    case StorageResult::kNamespaceNotFound:
    case StorageResult::kKeyNotFound:
      return modules::modes::makeModeNotFoundResult();
    case StorageResult::kInvalidPayload:
    case StorageResult::kIncompatibleSchema:
      return {modules::modes::ModeStoreLoadStatus::kInvalidMode,
              modules::modes::NORMAL};
    case StorageResult::kReadFailure:
    case StorageResult::kWriteFailure:
    case StorageResult::kNoChange:
    case StorageResult::kResetApplied:
    case StorageResult::kIgnoredByWritePolicy:
      return {modules::modes::ModeStoreLoadStatus::kFailure,
              modules::modes::NORMAL};
  }

  return {modules::modes::ModeStoreLoadStatus::kFailure,
          modules::modes::NORMAL};
}

modules::modes::ModeStoreSaveResult StorageModeStore::saveMode(
    modules::modes::OperationalMode mode) {
  const StorageResult result = storageService_.saveMode(mode);
  switch (result) {
    case StorageResult::kSuccess:
      return modules::modes::ModeStoreSaveResult::kSuccess;
    case StorageResult::kInvalidPayload:
      return modules::modes::ModeStoreSaveResult::kInvalidMode;
    case StorageResult::kNamespaceNotFound:
    case StorageResult::kKeyNotFound:
    case StorageResult::kIncompatibleSchema:
    case StorageResult::kReadFailure:
    case StorageResult::kWriteFailure:
    case StorageResult::kNoChange:
    case StorageResult::kResetApplied:
    case StorageResult::kIgnoredByWritePolicy:
      return modules::modes::ModeStoreSaveResult::kFailure;
  }

  return modules::modes::ModeStoreSaveResult::kFailure;
}

bool runStorageFlushTask(void* context) {
  if (context == nullptr) {
    return false;
  }

  StorageService* storage = static_cast<StorageService*>(context);
  const StorageResult result = storage->flushAllDirty();
  return result == StorageResult::kSuccess ||
         result == StorageResult::kNoChange;
}

}  // namespace reeflow::storage

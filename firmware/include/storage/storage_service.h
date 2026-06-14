#pragma once

#include "config/config_manager.h"
#include "core/events/event_bus.h"
#include "modules/lighting/lighting_profile.h"
#include "modules/modes/mode_store.h"
#include "storage/storage_backend.h"
#include "storage/storage_events.h"
#include "storage/storage_types.h"
#include "storage/storage_write_policy.h"

namespace reeflow::storage {

struct LightingPersistedState {
  config::LightingConfig config;
  modules::lighting::LightingProfile
      profiles[modules::lighting::kLightingMaxProfilesInMemory];
  uint8_t profileCount;
  uint8_t currentProfileIndex;
};

class StorageService {
 public:
  explicit StorageService(StorageBackend& backend);
  StorageService(StorageBackend& backend, core::events::EventBus& eventBus);

  StorageBackend& backend() { return backend_; }
  const StorageBackend& backend() const { return backend_; }

  StorageResult saveWifi(const config::WifiConfig& wifi,
                         uint32_t timestampMillis = 0);
  StorageResult restoreWifi(config::ConfigManager& configManager,
                            uint32_t timestampMillis = 0);

  StorageResult saveMqtt(const config::MqttConfig& mqtt,
                         uint32_t timestampMillis = 0);
  StorageResult restoreMqtt(config::ConfigManager& configManager,
                            uint32_t timestampMillis = 0);

  StorageResult saveTemperature(const config::TemperatureConfig& temperature,
                                uint32_t timestampMillis = 0);
  StorageResult restoreTemperature(config::ConfigManager& configManager,
                                   uint32_t timestampMillis = 0);

  StorageResult saveAto(const config::AtoConfig& ato,
                        uint32_t timestampMillis = 0);
  StorageResult restoreAto(config::ConfigManager& configManager,
                           uint32_t timestampMillis = 0);

  StorageResult saveTimers(const config::TimersConfig& timers,
                           uint32_t timestampMillis = 0);
  StorageResult restoreTimers(config::ConfigManager& configManager,
                              uint32_t timestampMillis = 0);

  StorageResult saveCalibrations(
      const config::CalibrationsConfig& calibrations,
      uint32_t timestampMillis = 0);
  StorageResult restoreCalibrations(config::ConfigManager& configManager,
                                    uint32_t timestampMillis = 0);

  StorageResult saveLighting(const LightingPersistedState& lighting,
                             uint32_t timestampMillis = 0);
  StorageResult restoreLighting(config::ConfigManager& configManager,
                                LightingPersistedState& lighting,
                                uint32_t timestampMillis = 0);

  StorageResult saveMode(modules::modes::OperationalMode mode,
                         uint32_t timestampMillis = 0);
  StorageResult restoreMode(modules::modes::OperationalMode& mode,
                            uint32_t timestampMillis = 0);

  StorageResult markConfigChanged(const config::ConfigManager& configManager,
                                  StorageDomain domain);
  StorageResult markLightingChanged(const LightingPersistedState& lighting);
  StorageResult flushDirty(StorageDomain domain,
                           uint32_t timestampMillis = 0);
  StorageResult flushAllDirty(uint32_t timestampMillis = 0);
  uint8_t dirtyCount() const;

 private:
  StorageResult markPayloadChanged(StorageDomain domain, const void* data,
                                   size_t dataSize);
  StorageResult savePayload(StorageDomain domain, const void* data,
                            size_t dataSize, uint32_t timestampMillis);
  StorageResult makeStoredPayload(StorageDomain domain, const void* data,
                                  size_t dataSize, uint8_t* payload,
                                  size_t& payloadSize) const;
  StorageResult readPayload(StorageDomain domain, void* data,
                            size_t dataSize) const;
  void publishConfigEvent(StorageEventType eventType, StorageDomain domain,
                          StorageResult result, uint32_t timestampMillis,
                          const char* reason);

  StorageBackend& backend_;
  core::events::EventBus& eventBus_;
  StorageWritePolicy writePolicy_;
};

bool runStorageFlushTask(void* context);

class StorageModeStore final : public modules::modes::ModeStore {
 public:
  explicit StorageModeStore(StorageService& storageService);

  modules::modes::ModeStoreLoadResult loadMode() override;
  modules::modes::ModeStoreSaveResult saveMode(
      modules::modes::OperationalMode mode) override;

 private:
  StorageService& storageService_;
};

}  // namespace reeflow::storage

#include <assert.h>
#include <string.h>

#include "app/core_app.h"
#include "config/config_manager.h"
#include "core/platform/core_platform.h"
#include "core/state/system_state.h"
#include "fakes/fake_internet_probe.h"
#include "fakes/fake_lighting_pwm_controller.h"
#include "fakes/fake_log_sink.h"
#include "fakes/fake_ntp_client.h"
#include "fakes/fake_relay_controller.h"
#include "fakes/fake_storage_backend.h"
#include "fakes/fake_temperature_sensor.h"
#include "fakes/fake_time_source.h"
#include "fakes/fake_watchdog_backend.h"
#include "fakes/fake_water_level_sensor.h"
#include "fakes/fake_wifi_adapter.h"
#include "network/network_heartbeat.h"
#include "network/network_status_service.h"
#include "network/ntp_service.h"
#include "network/wifi_service.h"
#include "storage/storage_service.h"

namespace {

using reeflow::app::CoreApp;
using reeflow::config::ConfigManager;
using reeflow::core::events::EventBus;
using reeflow::core::platform::CorePlatform;
using reeflow::network::NetworkHeartbeat;
using reeflow::network::NetworkStatusService;
using reeflow::network::NtpService;
using reeflow::network::WifiService;
using reeflow::storage::StorageModeStore;
using reeflow::storage::StorageResult;
using reeflow::storage::StorageService;
using reeflow::test::fakes::FakeInternetProbe;
using reeflow::test::fakes::FakeLightingPwmController;
using reeflow::test::fakes::FakeLogSink;
using reeflow::test::fakes::FakeNtpClient;
using reeflow::test::fakes::FakeRelayController;
using reeflow::test::fakes::FakeStorageBackend;
using reeflow::test::fakes::FakeTemperatureSensor;
using reeflow::test::fakes::FakeTimeSource;
using reeflow::test::fakes::FakeWatchdogBackend;
using reeflow::test::fakes::FakeWaterLevelSensor;
using reeflow::test::fakes::FakeWifiAdapter;

struct TestContext {
  FakeTimeSource timeSource;
  FakeLogSink logSink;
  FakeWatchdogBackend watchdogBackend;
  CorePlatform platform;
  EventBus eventBus;
  ConfigManager configManager;
  FakeTemperatureSensor temperatureSensor;
  FakeWaterLevelSensor waterLevelSensor;
  FakeRelayController relayController;
  FakeLightingPwmController lightingController;
  FakeStorageBackend storageBackend;
  StorageService storageService;
  StorageModeStore modeStore;
  FakeWifiAdapter wifiAdapter;
  FakeNtpClient ntpClient;
  FakeInternetProbe internetProbe;
  WifiService wifiService;
  NtpService ntpService;
  NetworkStatusService networkStatusService;
  NetworkHeartbeat networkHeartbeat;
  CoreApp app;

  TestContext()
      : platform(timeSource, logSink, watchdogBackend),
        configManager(eventBus),
        storageService(storageBackend, eventBus),
        modeStore(storageService),
        wifiService(wifiAdapter, configManager, eventBus),
        ntpService(ntpClient, eventBus),
        networkStatusService(wifiAdapter, &internetProbe),
        networkHeartbeat(eventBus),
        app(platform, eventBus, configManager, temperatureSensor,
            waterLevelSensor, relayController, lightingController, modeStore,
            &storageService, &wifiService, &ntpService, &networkStatusService,
            &networkHeartbeat) {}
};

void saveWifi(TestContext& context, const char* ssid, const char* password) {
  reeflow::config::WifiConfig wifi = {};
  wifi.enabled = true;
  strncpy(wifi.ssid, ssid, sizeof(wifi.ssid) - 1);
  strncpy(wifi.password, password, sizeof(wifi.password) - 1);
  assert(context.storageService.saveWifi(wifi) == StorageResult::kSuccess);
}

bool logsContain(const TestContext& context, const char* text) {
  for (const std::string& message : context.logSink.messages()) {
    if (message.find(text) != std::string::npos) {
      return true;
    }
  }
  return false;
}

void advanceAndRun(TestContext& context, uint32_t millis) {
  context.timeSource.advanceMillis(millis);
  const auto result = context.app.loopOnce();
  (void)result;
}

void testBootWithRestoredWifiRunsNetworkTasks() {
  TestContext context;
  saveWifi(context, "reef-ap", "super-secret-password");
  context.wifiAdapter.simulateSuccessfulConnection("192.168.1.44", -49);
  context.ntpClient.simulateSynced(1700000000UL);
  context.internetProbe.simulateAvailable();

  assert(context.app.setup());
  assert(context.wifiAdapter.beginCallCount() == 1);
  assert(!logsContain(context, "super-secret-password"));

  advanceAndRun(context, 1000);
  assert(context.wifiAdapter.connectCallCount() == 1);
  assert(reeflow::core::state::currentSystemState().network.wifiConnected);
  assert(strcmp(reeflow::core::state::currentSystemState().network.ipAddress,
                "192.168.1.44") == 0);

  advanceAndRun(context, 4000);
  assert(context.ntpClient.syncCallCount() == 1);
  assert(context.internetProbe.probeCallCount() == 1);
  assert(reeflow::core::state::currentSystemState().network.internetAvailable);
  assert(reeflow::core::state::currentSystemState().network.rssi == -49);

  advanceAndRun(context, 25000);
  assert(reeflow::core::state::currentSystemState().network.lastHeartbeat ==
         30000);
  assert(context.watchdogBackend.feedCalls() > 0);
}

void testBootWithoutWifiConfigDoesNotFailOrConnect() {
  TestContext context;

  assert(context.app.setup());
  advanceAndRun(context, 1000);

  assert(context.wifiAdapter.connectCallCount() == 0);
  assert(!reeflow::core::state::currentSystemState().network.wifiConnected);
  assert(!reeflow::core::state::currentSystemState().network.mqttConnected);
  assert(context.relayController.recordedCallCount() == 1);
  assert(logsContain(context, "wifi not configured"));
}

void testInvalidCredentialDoesNotFailBootOrLeakSecrets() {
  TestContext context;
  saveWifi(context, "reef-ap", "wrong-secret-token");
  context.wifiAdapter.simulateAuthenticationFailure();

  assert(context.app.setup());
  advanceAndRun(context, 1000);

  assert(context.wifiAdapter.connectCallCount() == 1);
  assert(!reeflow::core::state::currentSystemState().network.wifiConnected);
  assert(!reeflow::core::state::currentSystemState().network.mqttConnected);
  assert(logsContain(context, "wifi reconnect failed"));
  assert(!logsContain(context, "wrong-secret-token"));
}

void testAccessPointUnavailableDoesNotBlockLocalScheduler() {
  TestContext context;
  saveWifi(context, "missing-ap", "local-password");
  context.wifiAdapter.simulateAccessPointUnavailable();

  assert(context.app.setup());
  advanceAndRun(context, 5000);

  assert(context.wifiAdapter.connectCallCount() == 1);
  assert(context.temperatureSensor.readCount() > 0);
  assert(context.waterLevelSensor.readCount() > 0);
  assert(context.watchdogBackend.feedCalls() > 0);
  assert(!reeflow::core::state::currentSystemState().network.wifiConnected);
}

}  // namespace

int main() {
  testBootWithRestoredWifiRunsNetworkTasks();
  testBootWithoutWifiConfigDoesNotFailOrConnect();
  testInvalidCredentialDoesNotFailBootOrLeakSecrets();
  testAccessPointUnavailableDoesNotBlockLocalScheduler();
  return 0;
}

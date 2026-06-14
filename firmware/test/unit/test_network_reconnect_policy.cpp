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
using reeflow::core::events::EventBus;
using reeflow::network::NetworkConfig;
using reeflow::network::WifiConnectResult;
using reeflow::network::WifiReconnectPolicy;
using reeflow::network::WifiService;
using reeflow::network::makeWifiReconnectPolicy;
using reeflow::test::fakes::FakeWifiAdapter;

void enableWifi(ConfigManager& configManager) {
  auto wifi = configManager.wifi();
  wifi.enabled = true;
  strcpy(wifi.ssid, "reeflow");
  strcpy(wifi.password, "secret");
  assert(configManager.updateWifi(wifi));
}

void testReconnectPolicyUsesConfiguredBounds() {
  NetworkConfig config = {};
  config.reconnectInitialBackoffMillis = 250;
  config.reconnectMaxBackoffMillis = 2000;

  const WifiReconnectPolicy policy = makeWifiReconnectPolicy(config);

  assert(policy.initialBackoffMillis == 250);
  assert(policy.maxBackoffMillis == 2000);
}

void testWifiServiceAppliesProgressiveBackoffLimit() {
  EventBus eventBus;
  ConfigManager configManager(eventBus);
  FakeWifiAdapter adapter;
  NetworkConfig config = {};
  config.wifiConnectTimeoutMillis = 1000;
  config.reconnectInitialBackoffMillis = 100;
  config.reconnectMaxBackoffMillis = 250;
  config.heartbeatIntervalMillis = 30000;
  config.statusUpdateIntervalMillis = 5000;
  config.rssiUpdateIntervalMillis = 10000;
  config.ntpSyncTimeoutMillis = 10000;
  config.primaryNtpServer = "pool.ntp.org";
  config.secondaryNtpServer = "time.nist.gov";
  WifiService service(adapter, configManager, eventBus, config);

  reeflow::core::state::resetSystemState();
  reeflow::core::state::setSystemStateEventBus(eventBus);
  enableWifi(configManager);
  adapter.simulateAccessPointUnavailable();

  service.tick(0);
  assert(service.snapshot().lastConnectResult ==
         WifiConnectResult::kAccessPointUnavailable);
  assert(service.snapshot().currentBackoffMillis == 100);
  assert(service.snapshot().nextReconnectAtMillis == 100);

  service.tick(100);
  assert(service.snapshot().currentBackoffMillis == 200);
  assert(service.snapshot().nextReconnectAtMillis == 300);

  service.tick(300);
  assert(service.snapshot().currentBackoffMillis == 250);
  assert(service.snapshot().nextReconnectAtMillis == 550);

  service.tick(550);
  assert(service.snapshot().currentBackoffMillis == 250);
  assert(service.snapshot().nextReconnectAtMillis == 800);
}

void testSuccessfulConnectionResetsBackoff() {
  EventBus eventBus;
  ConfigManager configManager(eventBus);
  FakeWifiAdapter adapter;
  NetworkConfig config = {};
  config.wifiConnectTimeoutMillis = 1000;
  config.reconnectInitialBackoffMillis = 100;
  config.reconnectMaxBackoffMillis = 250;
  config.heartbeatIntervalMillis = 30000;
  config.statusUpdateIntervalMillis = 5000;
  config.rssiUpdateIntervalMillis = 10000;
  config.ntpSyncTimeoutMillis = 10000;
  config.primaryNtpServer = "pool.ntp.org";
  config.secondaryNtpServer = "time.nist.gov";
  WifiService service(adapter, configManager, eventBus, config);

  reeflow::core::state::resetSystemState();
  reeflow::core::state::setSystemStateEventBus(eventBus);
  enableWifi(configManager);
  adapter.simulateAccessPointUnavailable();

  service.tick(0);
  service.tick(100);
  assert(service.snapshot().currentBackoffMillis == 200);

  adapter.simulateSuccessfulConnection("192.168.1.20", -55);
  service.tick(300);

  assert(service.snapshot().lastConnectResult == WifiConnectResult::kConnected);
  assert(service.snapshot().currentBackoffMillis == 0);
  assert(service.snapshot().nextReconnectAtMillis == 0);
}

}  // namespace

int main() {
  testReconnectPolicyUsesConfiguredBounds();
  testWifiServiceAppliesProgressiveBackoffLimit();
  testSuccessfulConnectionResetsBackoff();
  return 0;
}

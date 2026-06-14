#include <assert.h>
#include <string.h>

#include "core/state/system_state.h"
#include "fakes/fake_internet_probe.h"
#include "fakes/fake_wifi_adapter.h"
#include "network/network_config.h"
#include "network/network_status_service.h"

namespace {

using reeflow::core::state::SystemState;
using reeflow::network::InternetStatus;
using reeflow::network::NetworkConfig;
using reeflow::network::NetworkStatusService;
using reeflow::network::NtpStatus;
using reeflow::network::NtpSyncStatus;
using reeflow::network::makeDefaultNetworkConfig;
using reeflow::test::fakes::FakeInternetProbe;
using reeflow::test::fakes::FakeWifiAdapter;

NetworkConfig fastConfig() {
  NetworkConfig config = makeDefaultNetworkConfig();
  config.statusUpdateIntervalMillis = 1000;
  config.rssiUpdateIntervalMillis = 2000;
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
  assert(after.alerts.activeAlertCount == before.alerts.activeAlertCount);
}

void testConnectedWifiUpdatesNetworkFieldsAndInternet() {
  reeflow::core::state::resetSystemState();
  FakeWifiAdapter wifi;
  FakeInternetProbe internet;
  NetworkStatusService service(wifi, &internet, fastConfig());

  wifi.setConnectedStatus("192.168.1.20", -55);
  internet.simulateAvailable();
  service.tick(1000);

  const auto& network = reeflow::core::state::currentSystemState().network;
  assert(network.wifiConnected);
  assert(network.internetAvailable);
  assert(strcmp(network.ipAddress, "192.168.1.20") == 0);
  assert(network.rssi == -55);
  assert(!network.mqttConnected);
  assert(internet.probeCallCount() == 1);
  assert(service.snapshot().internet.status == InternetStatus::kAvailable);
}

void testDisconnectedWifiProducesDeterministicStatusWithoutProbe() {
  reeflow::core::state::resetSystemState();
  FakeWifiAdapter wifi;
  FakeInternetProbe internet;
  NetworkStatusService service(wifi, &internet, fastConfig());

  wifi.setConnectedStatus("192.168.1.20", -55);
  internet.simulateAvailable();
  service.tick(0);
  wifi.simulateConnectionDrop();
  service.tick(1000);

  const auto& network = reeflow::core::state::currentSystemState().network;
  assert(!network.wifiConnected);
  assert(!network.internetAvailable);
  assert(network.ipAddress[0] == '\0');
  assert(network.rssi == 0);
  assert(!network.mqttConnected);
  assert(internet.probeCallCount() == 1);
  assert(service.snapshot().internet.status == InternetStatus::kUnavailable);
}

void testUnavailableInternetDoesNotAffectLocalAutomationState() {
  reeflow::core::state::resetSystemState();
  FakeWifiAdapter wifi;
  FakeInternetProbe internet;
  NetworkStatusService service(wifi, &internet, fastConfig());
  wifi.setConnectedStatus("192.168.1.20", -55);
  internet.simulateUnavailable();

  const SystemState before = reeflow::core::state::currentSystemState();
  service.tick(1000);

  const auto& network = reeflow::core::state::currentSystemState().network;
  assert(network.wifiConnected);
  assert(!network.internetAvailable);
  assertUnrelatedBlocksMatch(before);
}

void testStatusAndRssiIntervalsAreConfigurable() {
  reeflow::core::state::resetSystemState();
  FakeWifiAdapter wifi;
  FakeInternetProbe internet;
  NetworkStatusService service(wifi, &internet, fastConfig());
  wifi.setConnectedStatus("192.168.1.20", -55);
  internet.simulateAvailable();

  service.tick(0);
  wifi.setConnectedStatus("192.168.1.21", -70);
  service.tick(999);

  auto network = reeflow::core::state::currentSystemState().network;
  assert(strcmp(network.ipAddress, "192.168.1.20") == 0);
  assert(network.rssi == -55);
  assert(internet.probeCallCount() == 1);

  service.tick(1000);
  network = reeflow::core::state::currentSystemState().network;
  assert(strcmp(network.ipAddress, "192.168.1.20") == 0);
  assert(network.rssi == -55);
  assert(internet.probeCallCount() == 2);

  service.tick(2000);
  network = reeflow::core::state::currentSystemState().network;
  assert(strcmp(network.ipAddress, "192.168.1.21") == 0);
  assert(network.rssi == -70);
}

void testNtpStatusIsCarriedInSnapshotOnly() {
  reeflow::core::state::resetSystemState();
  FakeWifiAdapter wifi;
  NetworkStatusService service(wifi, nullptr, fastConfig());
  wifi.setConnectedStatus("192.168.1.20", -55);
  NtpStatus ntp = {};
  ntp.status = NtpSyncStatus::kSynced;
  ntp.timestamp = {true, 1710000000};

  service.tick(1000, ntp);

  assert(service.snapshot().ntp.status == NtpSyncStatus::kSynced);
  assert(service.snapshot().ntp.timestamp.available);
  assert(reeflow::core::state::currentSystemState().network.lastHeartbeat == 0);
}

}  // namespace

int main() {
  testConnectedWifiUpdatesNetworkFieldsAndInternet();
  testDisconnectedWifiProducesDeterministicStatusWithoutProbe();
  testUnavailableInternetDoesNotAffectLocalAutomationState();
  testStatusAndRssiIntervalsAreConfigurable();
  testNtpStatusIsCarriedInSnapshotOnly();
  return 0;
}

#include <assert.h>
#include <string.h>

#include "core/events/event_bus.h"
#include "fakes/fake_internet_probe.h"
#include "fakes/fake_ntp_client.h"
#include "fakes/fake_wifi_adapter.h"
#include "network/network_config.h"
#include "network/network_events.h"
#include "network/network_heartbeat.h"
#include "network/network_status_service.h"
#include "network/network_types.h"
#include "network/ntp_service.h"
#include "network/wifi_service.h"

namespace {

using reeflow::core::events::Event;
using reeflow::core::events::EventBus;
using reeflow::core::events::EventType;
using reeflow::network::InternetStatus;
using reeflow::network::NetworkEvent;
using reeflow::network::NetworkEventType;
using reeflow::network::NtpSyncResult;
using reeflow::network::NtpSyncStatus;
using reeflow::network::WifiConnectResult;
using reeflow::network::WifiConnectionState;
using reeflow::network::WifiCredentials;
using reeflow::network::WifiDisconnectReason;
using reeflow::network::makeDefaultNetworkConfig;
using reeflow::network::networkCoreEventType;
using reeflow::network::networkEventName;
using reeflow::network::publishNetworkEvent;
using reeflow::network::validNetworkConfig;
using reeflow::test::fakes::FakeInternetProbe;
using reeflow::test::fakes::FakeNtpClient;
using reeflow::test::fakes::FakeWifiAdapter;

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

void testWifiTypesRepresentExpectedStatesAndFailures() {
  assert(WifiConnectionState::kDisabled != WifiConnectionState::kConnected);
  assert(WifiConnectionState::kDisconnected != WifiConnectionState::kConnected);
  assert(WifiConnectionState::kReconnectPending !=
         WifiConnectionState::kFailed);

  assert(WifiConnectResult::kCredentialMissing !=
         WifiConnectResult::kAuthenticationFailed);
  assert(WifiConnectResult::kAccessPointUnavailable !=
         WifiConnectResult::kTimeout);
  assert(WifiDisconnectReason::kSignalLost !=
         WifiDisconnectReason::kManualDisconnect);
}

void testInternetAndNtpTypesRepresentExpectedStates() {
  assert(InternetStatus::kAvailable != InternetStatus::kUnavailable);
  assert(InternetStatus::kDnsFailure != InternetStatus::kTimeout);
  assert(NtpSyncStatus::kSynced != NtpSyncStatus::kTimeout);
  assert(NtpSyncResult::kServerUnavailable != NtpSyncResult::kClientFailure);
}

void testDefaultNetworkConfigIsDeterministicAndValid() {
  const auto config = makeDefaultNetworkConfig();

  assert(validNetworkConfig(config));
  assert(config.wifiConnectTimeoutMillis == 15000);
  assert(config.reconnectInitialBackoffMillis == 1000);
  assert(config.reconnectMaxBackoffMillis == 60000);
  assert(config.heartbeatIntervalMillis == 30000);
  assert(config.statusUpdateIntervalMillis == 5000);
  assert(config.rssiUpdateIntervalMillis == 10000);
  assert(strcmp(config.primaryNtpServer, "pool.ntp.org") == 0);
}

void testNetworkEventsExposeCanonicalNamesAndLocalBusTypes() {
  assert(strcmp(networkEventName(NetworkEventType::kWifiConnected),
                "WIFI_CONNECTED") == 0);
  assert(strcmp(networkEventName(NetworkEventType::kWifiDisconnected),
                "WIFI_DISCONNECTED") == 0);
  assert(strcmp(networkEventName(NetworkEventType::kWifiReconnecting),
                "WIFI_RECONNECTING") == 0);
  assert(strcmp(networkEventName(NetworkEventType::kWifiReconnectFailed),
                "WIFI_RECONNECT_FAILED") == 0);
  assert(strcmp(networkEventName(NetworkEventType::kNtpSynced),
                "NTP_SYNCED") == 0);
  assert(strcmp(networkEventName(NetworkEventType::kNtpSyncFailed),
                "NTP_SYNC_FAILED") == 0);
  assert(strcmp(networkEventName(NetworkEventType::kNetworkHeartbeat),
                "NETWORK_HEARTBEAT") == 0);

  assert(networkCoreEventType(NetworkEventType::kWifiConnected) ==
         EventType::kWifiConnected);
  assert(networkCoreEventType(NetworkEventType::kWifiDisconnected) ==
         EventType::kWifiDisconnected);
  assert(networkCoreEventType(NetworkEventType::kWifiReconnecting) ==
         EventType::kWifiReconnecting);
  assert(networkCoreEventType(NetworkEventType::kWifiReconnectFailed) ==
         EventType::kWifiReconnectFailed);
  assert(networkCoreEventType(NetworkEventType::kNtpSynced) ==
         EventType::kNtpSynced);
  assert(networkCoreEventType(NetworkEventType::kNtpSyncFailed) ==
         EventType::kNtpSyncFailed);
  assert(networkCoreEventType(NetworkEventType::kNetworkHeartbeat) ==
         EventType::kNetworkHeartbeat);
}

void testNetworkEventPublishesOnlyToLocalEventBus() {
  EventBus bus;
  EventRecorder recorder = {};
  bus.subscribe(EventType::kWifiConnected, recordEvent, &recorder);

  NetworkEvent event = {};
  event.type = NetworkEventType::kWifiConnected;
  event.occurredAtMillis = 1000;

  const auto result = publishNetworkEvent(event, bus);

  assert(result.deliveredCount == 1);
  assert(result.failedCount == 0);
  assert(recorder.count == 1);
  assert(recorder.events[0].type == EventType::kWifiConnected);
  assert(recorder.events[0].payload == &event);
}

void testFakeWifiAdapterSimulatesSuccessFailuresAndDrop() {
  FakeWifiAdapter adapter;
  adapter.simulateSuccessfulConnection("192.168.1.20", -55);

  const WifiCredentials credentials = {"reeflow", "secret"};
  assert(adapter.connect(credentials, 15000) == WifiConnectResult::kConnected);
  assert(adapter.connectCallCount() == 1);
  assert(adapter.lastTimeoutMillis() == 15000);
  assert(adapter.status().state == WifiConnectionState::kConnected);
  assert(adapter.status().hasIpAddress);
  assert(strcmp(adapter.ipAddress(), "192.168.1.20") == 0);
  assert(adapter.status().hasRssi);
  assert(adapter.rssi() == -55);

  adapter.simulateAccessPointUnavailable();
  assert(adapter.connect(credentials, 15000) ==
         WifiConnectResult::kAccessPointUnavailable);
  assert(adapter.lastDisconnectReason() ==
         WifiDisconnectReason::kAccessPointUnavailable);

  adapter.simulateConnectionDrop();
  assert(adapter.status().state == WifiConnectionState::kDisconnected);
  assert(adapter.lastDisconnectReason() == WifiDisconnectReason::kSignalLost);

  const WifiCredentials missingCredentials = {"", ""};
  assert(adapter.connect(missingCredentials, 15000) ==
         WifiConnectResult::kCredentialMissing);
}

void testFakeNtpClientSimulatesSyncedAndTimeout() {
  FakeNtpClient client;
  const auto config = makeDefaultNetworkConfig();

  client.simulateSynced(1710000000);
  assert(client.startSync(config, 4000) == NtpSyncResult::kSynced);
  assert(client.syncCallCount() == 1);
  assert(client.status().status == NtpSyncStatus::kSynced);
  assert(client.timestamp().available);
  assert(client.timestamp().epochSeconds == 1710000000);

  client.simulateTimeout();
  assert(client.startSync(config, 5000) == NtpSyncResult::kTimeout);
  assert(client.status().status == NtpSyncStatus::kTimeout);
  assert(!client.timestamp().available);
}

void testFakeInternetProbeSimulatesConnectivityOutcomes() {
  FakeInternetProbe probe;

  probe.simulateAvailable();
  assert(probe.probe(1000).status == InternetStatus::kAvailable);
  probe.simulateUnavailable();
  assert(probe.probe(2000).status == InternetStatus::kUnavailable);
  probe.simulateDnsFailure();
  assert(probe.probe(3000).status == InternetStatus::kDnsFailure);
  probe.simulateTimeout();
  const auto result = probe.probe(4000);
  assert(result.status == InternetStatus::kTimeout);
  assert(result.checkedAtMillis == 4000);
  assert(probe.probeCallCount() == 4);
}

}  // namespace

int main() {
  testWifiTypesRepresentExpectedStatesAndFailures();
  testInternetAndNtpTypesRepresentExpectedStates();
  testDefaultNetworkConfigIsDeterministicAndValid();
  testNetworkEventsExposeCanonicalNamesAndLocalBusTypes();
  testNetworkEventPublishesOnlyToLocalEventBus();
  testFakeWifiAdapterSimulatesSuccessFailuresAndDrop();
  testFakeNtpClientSimulatesSyncedAndTimeout();
  testFakeInternetProbeSimulatesConnectivityOutcomes();
  return 0;
}

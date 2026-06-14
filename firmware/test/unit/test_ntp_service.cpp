#include <assert.h>

#include "core/events/event_bus.h"
#include "core/state/system_state.h"
#include "fakes/fake_ntp_client.h"
#include "network/network_config.h"
#include "network/ntp_service.h"

namespace {

using reeflow::core::events::Event;
using reeflow::core::events::EventBus;
using reeflow::core::events::EventType;
using reeflow::core::state::NetworkState;
using reeflow::network::NetworkConfig;
using reeflow::network::NtpService;
using reeflow::network::NtpSyncResult;
using reeflow::network::NtpSyncStatus;
using reeflow::network::makeDefaultNetworkConfig;
using reeflow::test::fakes::FakeNtpClient;

struct EventRecorder {
  Event events[8];
  uint8_t count;
};

bool recordEvent(const Event& event, void* context) {
  EventRecorder* recorder = static_cast<EventRecorder*>(context);
  recorder->events[recorder->count++] = event;
  return true;
}

struct Fixture {
  FakeNtpClient client;
  EventBus eventBus;
  NetworkConfig networkConfig;
  NtpService service;
  EventRecorder syncedEvents;
  EventRecorder failedEvents;

  Fixture()
      : networkConfig(makeDefaultNetworkConfig()),
        service(client, eventBus, networkConfig),
        syncedEvents({}),
        failedEvents({}) {
    reeflow::core::state::resetSystemState();
    reeflow::core::state::setSystemStateEventBus(eventBus);
    eventBus.subscribe(EventType::kNtpSynced, recordEvent, &syncedEvents);
    eventBus.subscribe(EventType::kNtpSyncFailed, recordEvent, &failedEvents);
  }
};

void setWifiConnected(bool connected) {
  NetworkState network = reeflow::core::state::currentSystemState().network;
  network.wifiConnected = connected;
  reeflow::core::state::updateNetworkState(network);
}

void testNtpDoesNotStartWithoutWifi() {
  Fixture fixture;

  fixture.service.tick(1000);

  assert(fixture.client.syncCallCount() == 0);
  assert(fixture.service.snapshot().lastSyncResult ==
         NtpSyncResult::kNetworkUnavailable);
  assert(fixture.service.snapshot().status.status ==
         NtpSyncStatus::kNotStarted);
  assert(fixture.syncedEvents.count == 0);
  assert(fixture.failedEvents.count == 0);
}

void testNtpSyncedPublishesLocalEvent() {
  Fixture fixture;
  setWifiConnected(true);
  fixture.client.simulateSynced(1710000000);

  fixture.service.tick(1000);

  assert(fixture.client.syncCallCount() == 1);
  assert(fixture.service.snapshot().lastSyncResult == NtpSyncResult::kSynced);
  assert(fixture.service.snapshot().status.status == NtpSyncStatus::kSynced);
  assert(fixture.service.snapshot().status.timestamp.available);
  assert(fixture.service.snapshot().status.timestamp.epochSeconds ==
         1710000000);
  assert(fixture.syncedEvents.count == 1);
  assert(fixture.syncedEvents.events[0].type == EventType::kNtpSynced);
  assert(fixture.failedEvents.count == 0);
}

void testNtpTimeoutIsNonBlockingAndPublishesFailure() {
  Fixture fixture;
  setWifiConnected(true);
  fixture.client.simulateSyncing();

  fixture.service.tick(0);
  fixture.service.tick(9999);

  assert(fixture.client.syncCallCount() == 1);
  assert(fixture.service.snapshot().syncInProgress);
  assert(fixture.failedEvents.count == 0);

  fixture.service.tick(10000);

  assert(!fixture.service.snapshot().syncInProgress);
  assert(fixture.service.snapshot().lastSyncResult == NtpSyncResult::kTimeout);
  assert(fixture.service.snapshot().status.status == NtpSyncStatus::kTimeout);
  assert(fixture.failedEvents.count == 1);
  assert(fixture.failedEvents.events[0].type == EventType::kNtpSyncFailed);
}

void testNtpServerUnavailableIsRepresented() {
  Fixture fixture;
  setWifiConnected(true);
  fixture.client.simulateServerUnavailable();

  fixture.service.tick(2000);

  assert(fixture.service.snapshot().lastSyncResult ==
         NtpSyncResult::kServerUnavailable);
  assert(fixture.service.snapshot().status.status ==
         NtpSyncStatus::kServerUnavailable);
  assert(fixture.failedEvents.count == 1);
}

void testLossOfWifiDuringSyncDoesNotFatal() {
  Fixture fixture;
  setWifiConnected(true);
  fixture.client.simulateSyncing();
  fixture.service.tick(0);

  setWifiConnected(false);
  fixture.service.tick(1000);

  assert(fixture.client.syncCallCount() == 1);
  assert(!fixture.service.snapshot().syncInProgress);
  assert(fixture.service.snapshot().lastSyncResult ==
         NtpSyncResult::kNetworkUnavailable);
  assert(fixture.failedEvents.count == 1);
}

void testLocalStateContinuesUsingMonotonicFallbackWhenNtpFails() {
  Fixture fixture;
  setWifiConnected(true);
  fixture.client.simulateTimeout();
  const auto beforeHealth =
      reeflow::core::state::currentSystemState().systemHealth;

  fixture.service.tick(5000);

  const auto afterHealth =
      reeflow::core::state::currentSystemState().systemHealth;
  assert(afterHealth.uptime == beforeHealth.uptime);
  assert(afterHealth.lastReboot == beforeHealth.lastReboot);
  assert(!fixture.service.snapshot().status.timestamp.available);
}

void testSchedulerLikeTicksCanContinueDuringSyncAttempt() {
  Fixture fixture;
  setWifiConnected(true);
  fixture.client.simulateSyncing();
  uint8_t simulatedTaskTicks = 0;

  fixture.service.tick(0);
  simulatedTaskTicks += 1;
  fixture.service.tick(100);
  simulatedTaskTicks += 1;
  fixture.service.tick(200);
  simulatedTaskTicks += 1;

  assert(simulatedTaskTicks == 3);
  assert(fixture.client.syncCallCount() == 1);
  assert(fixture.service.snapshot().syncInProgress);
}

}  // namespace

int main() {
  testNtpDoesNotStartWithoutWifi();
  testNtpSyncedPublishesLocalEvent();
  testNtpTimeoutIsNonBlockingAndPublishesFailure();
  testNtpServerUnavailableIsRepresented();
  testLossOfWifiDuringSyncDoesNotFatal();
  testLocalStateContinuesUsingMonotonicFallbackWhenNtpFails();
  testSchedulerLikeTicksCanContinueDuringSyncAttempt();
  return 0;
}

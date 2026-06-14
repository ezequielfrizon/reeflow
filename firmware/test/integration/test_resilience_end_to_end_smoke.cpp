#include <assert.h>
#include <string.h>

#include "core/scheduler/task_scheduler.h"
#include "core/state/system_state.h"
#include "fakes/fake_power_cycle_context.h"
#include "fakes/fake_resilience_ato.h"
#include "fakes/fake_resilience_mqtt.h"
#include "fakes/fake_resilience_network.h"
#include "fakes/fake_resilience_sensors.h"
#include "resilience/resilience_report.h"
#include "resilience/resilience_scenarios.h"

namespace {

using reeflow::core::events::Event;
using reeflow::core::events::EventType;
using reeflow::core::scheduler::SchedulerRunResult;
using reeflow::core::state::NetworkState;
using reeflow::core::state::updateAtoState;
using reeflow::core::state::updateNetworkState;
using reeflow::resilience::ExpectedRecovery;
using reeflow::resilience::ResilienceReport;
using reeflow::resilience::ResilienceResult;
using reeflow::resilience::ResilienceScenarioId;
using reeflow::resilience::buildResilienceFinalReport;
using reeflow::resilience::findResilienceScenario;
using reeflow::resilience::resilienceScenarioAt;
using reeflow::resilience::resilienceScenarioCount;
using reeflow::modules::temperature::TemperatureSensorReadStatus;
using reeflow::modules::water_level::WaterLevelSensorReadStatus;
using reeflow::test::fakes::FakePowerCycleContext;
using reeflow::test::fakes::FakeResilienceAto;
using reeflow::test::fakes::FakeResilienceMqtt;
using reeflow::test::fakes::FakeResilienceNetwork;
using reeflow::test::fakes::FakeResilienceSensors;

struct SchedulerCounter {
  uint8_t runs;
};

bool countSchedulerRun(void* context) {
  SchedulerCounter* counter = static_cast<SchedulerCounter*>(context);
  if (counter == nullptr) {
    return false;
  }

  counter->runs += 1;
  return true;
}

void assertCanonicalScenariosExist() {
  assert(resilienceScenarioCount() == 7);
  assert(findResilienceScenario(ResilienceScenarioId::kUnexpectedReboot) !=
         nullptr);
  assert(findResilienceScenario(ResilienceScenarioId::kSimulatedPowerLoss) !=
         nullptr);
  assert(findResilienceScenario(ResilienceScenarioId::kWifiLoss) != nullptr);
  assert(findResilienceScenario(ResilienceScenarioId::kMqttLoss) != nullptr);
  assert(findResilienceScenario(
             ResilienceScenarioId::kTemperatureSensorOffline) != nullptr);
  assert(findResilienceScenario(
             ResilienceScenarioId::kWaterLevelSensorOffline) != nullptr);
  assert(findResilienceScenario(ResilienceScenarioId::kAtoStuck) != nullptr);

  for (size_t index = 0; index < resilienceScenarioCount(); ++index) {
    assert(resilienceScenarioAt(index).stepCount > 0);
    assert(resilienceScenarioAt(index).name[0] != '\0');
  }
}

void assertHarnessStartsWithoutHardwareAndCapturesState() {
  FakePowerCycleContext context;
  SchedulerCounter counter = {};

  assert(context.start());
  assert(context.started());
  assert(context.registerTask("resilience-smoke", 1000, countSchedulerRun,
                              &counter) !=
         reeflow::core::scheduler::kInvalidTaskId);

  SchedulerRunResult result = context.runSchedulerFor(1000);
  assert(result.executedCount == 1);
  assert(result.failedCount == 0);
  assert(counter.runs == 1);

  NetworkState network = context.state().network;
  network.wifiConnected = true;
  network.mqttConnected = false;
  network.internetAvailable = false;
  strncpy(network.ipAddress, "192.168.1.60", sizeof(network.ipAddress) - 1);
  updateNetworkState(network);

  assert(context.capturedEventCount() == 1);
  assert(context.capturedEvent(0).type == EventType::kSystemStateChanged);
  assert(context.state().network.wifiConnected);
}

void assertFakesStayUsableInHarness() {
  FakeResilienceNetwork network;
  FakeResilienceMqtt mqtt;
  FakeResilienceSensors sensors;
  FakeResilienceAto ato;

  network.startOnline();
  network.recoverWifi();
  assert(network.wifi().status().state ==
         reeflow::network::WifiConnectionState::kConnected);
  network.failWifi();
  assert(network.wifi().status().state ==
         reeflow::network::WifiConnectionState::kDisconnected);

  mqtt.startOnline();
  assert(mqtt.mqtt().connected());
  mqtt.failMqtt();
  assert(!mqtt.mqtt().connected());

  sensors.startOnline();
  assert(sensors.temperature().readTemperature().status ==
         TemperatureSensorReadStatus::kValid);
  assert(sensors.waterLevel().readLevel().status ==
         WaterLevelSensorReadStatus::kValid);
  sensors.failTemperature();
  assert(sensors.temperature().readTemperature().status !=
         TemperatureSensorReadStatus::kValid);
  sensors.failWaterLevel();
  assert(sensors.waterLevel().readLevel().status !=
         WaterLevelSensorReadStatus::kValid);

  ato.startAllowed();
  ato.simulatePumpRunning();
  assert(ato.pumpRunning());
  ato.simulateTimeout(2000);
  assert(!ato.pumpRunning());
  assert(ato.timedOut());
  updateAtoState(ato.snapshot());
  assert(reeflow::core::state::currentSystemState().ato.status ==
         reeflow::core::state::AtoStatus::kTimeout);
}

void assertReportIsDeterministicAndLocal() {
  FakePowerCycleContext context;
  ResilienceReport report;
  const auto* scenario =
      findResilienceScenario(ResilienceScenarioId::kWifiLoss);
  assert(scenario != nullptr);
  assert(scenario->expectedRecovery ==
         ExpectedRecovery::kConnectivityRecovered);

  assert(context.start());
  assert(report.beginScenario(*scenario, context.nowMillis(), context.state()));

  Event event = {};
  event.type = EventType::kWifiDisconnected;
  event.sequence = 7;
  assert(report.recordEvent(scenario->id, 1000, event));
  assert(report.completeScenario(scenario->id, ResilienceResult::kPassed,
                                 2000, context.state()));

  const auto* scenarioReport = report.scenarioReport(scenario->id);
  assert(scenarioReport != nullptr);
  assert(scenarioReport->result == ResilienceResult::kPassed);
  assert(scenarioReport->observedEventCount == 1);
  assert(strcmp(scenarioReport->expectedRecovery,
                "connectivity status recovered without blocking local "
                "automation") == 0);

  const auto summary = report.summary();
  assert(summary.scenarioCount == 1);
  assert(summary.passedCount == 1);
  assert(summary.failedCount == 0);
  assert(summary.notRunCount == 0);
}

EventType representativeEventFor(ResilienceScenarioId id) {
  switch (id) {
    case ResilienceScenarioId::kUnexpectedReboot:
    case ResilienceScenarioId::kSimulatedPowerLoss:
      return EventType::kSystemStateChanged;
    case ResilienceScenarioId::kWifiLoss:
      return EventType::kWifiDisconnected;
    case ResilienceScenarioId::kMqttLoss:
      return EventType::kMqttDisconnected;
    case ResilienceScenarioId::kTemperatureSensorOffline:
      return EventType::kTemperatureSensorOffline;
    case ResilienceScenarioId::kWaterLevelSensorOffline:
      return EventType::kWaterLevelSensorOffline;
    case ResilienceScenarioId::kAtoStuck:
      return EventType::kAtoTimeout;
  }
  return EventType::kSystemStateChanged;
}

void assertFinalReportCoversCanonicalScenarios() {
  FakePowerCycleContext context;
  ResilienceReport report;
  assert(context.start());

  for (size_t index = 0; index < resilienceScenarioCount(); ++index) {
    const auto& scenario = resilienceScenarioAt(index);
    assert(report.beginScenario(scenario, 1000 + index, context.state()));

    Event event = {};
    event.type = representativeEventFor(scenario.id);
    event.sequence = static_cast<uint32_t>(index + 1);
    assert(report.recordEvent(scenario.id, 1100 + index, event));

    Event alert = {};
    alert.type = EventType::kAlertRaised;
    alert.sequence = static_cast<uint32_t>(100 + index);
    assert(report.recordEvent(scenario.id, 1200 + index, alert));

    assert(report.completeScenario(scenario.id, ResilienceResult::kPassed,
                                   1300 + index, context.state()));
  }

  char output[4096] = {};
  assert(buildResilienceFinalReport(report, output, sizeof(output)));

  assert(strstr(output, "Phase 13 resilience final report") != nullptr);
  assert(strstr(output, "scenarios=7 passed=7 failed=0 not-run=0") !=
         nullptr);
  assert(strstr(output, "result=passed") != nullptr);
  assert(strstr(output, "events-observed=") != nullptr);
  assert(strstr(output, "alerts-observed=") != nullptr);
  assert(strstr(output, "validation-limitations=no hardware validation "
                        "executed") != nullptr);
  assert(strstr(output, "Pendente para Hardware Validation") != nullptr);

  for (size_t index = 0; index < resilienceScenarioCount(); ++index) {
    assert(strstr(output, resilienceScenarioAt(index).name) != nullptr);
  }
}

void assertFinalReportFailsWhenScenarioMissingOrFailed() {
  FakePowerCycleContext context;
  ResilienceReport incompleteReport;
  ResilienceReport failedReport;
  char output[512] = {};
  assert(context.start());

  const auto& firstScenario = resilienceScenarioAt(0);
  assert(incompleteReport.beginScenario(firstScenario, 1000, context.state()));
  assert(incompleteReport.completeScenario(firstScenario.id,
                                           ResilienceResult::kPassed, 1100,
                                           context.state()));
  assert(!buildResilienceFinalReport(incompleteReport, output,
                                     sizeof(output)));

  for (size_t index = 0; index < resilienceScenarioCount(); ++index) {
    const auto& scenario = resilienceScenarioAt(index);
    assert(failedReport.beginScenario(scenario, 1000 + index,
                                      context.state()));
    const ResilienceResult result =
        index == 0 ? ResilienceResult::kFailed : ResilienceResult::kPassed;
    assert(failedReport.completeScenario(scenario.id, result, 1300 + index,
                                         context.state()));
  }
  assert(!buildResilienceFinalReport(failedReport, output, sizeof(output)));
}

}  // namespace

int main() {
  assertCanonicalScenariosExist();
  assertHarnessStartsWithoutHardwareAndCapturesState();
  assertFakesStayUsableInHarness();
  assertReportIsDeterministicAndLocal();
  assertFinalReportCoversCanonicalScenarios();
  assertFinalReportFailsWhenScenarioMissingOrFailed();
  return 0;
}

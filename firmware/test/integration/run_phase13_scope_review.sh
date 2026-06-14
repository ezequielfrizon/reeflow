#!/usr/bin/env bash
set -euo pipefail

FIRMWARE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
REPO_DIR="$(cd "$FIRMWARE_DIR/.." && pwd)"

check_present() {
  local pattern="$1"
  local message="$2"
  shift 2

  if ! grep -RInE "$pattern" "$@" >/dev/null; then
    echo "$message" >&2
    exit 1
  fi
}

check_absent() {
  local pattern="$1"
  local message="$2"
  shift 2

  if grep -RInE "$pattern" "$@" >/dev/null; then
    echo "$message" >&2
    grep -RInE "$pattern" "$@" >&2
    exit 1
  fi
}

check_present \
  "ResilienceScenarioId|ResilienceScenarioDefinition|ResilienceReport" \
  "Phase 13 resilience contracts must exist." \
  "$FIRMWARE_DIR/include/resilience" \
  "$FIRMWARE_DIR/src/resilience"

check_present \
  "kUnexpectedReboot|kSimulatedPowerLoss|kWifiLoss|kMqttLoss|kTemperatureSensorOffline|kWaterLevelSensorOffline|kAtoStuck" \
  "FW-P13-000 must define all canonical resilience scenarios." \
  "$FIRMWARE_DIR/include/resilience" \
  "$FIRMWARE_DIR/src/resilience"

check_present \
  "FakePowerCycleContext|FakeResilienceNetwork|FakeResilienceMqtt|FakeResilienceSensors|FakeResilienceAto" \
  "FW-P13-000 must keep no-hardware fakes under firmware/test/fakes." \
  "$FIRMWARE_DIR/test/fakes"

check_present \
  "kUnexpectedReboot|kSimulatedPowerLoss|watchdogTriggered|UNEXPECTED_REBOOT|ResilienceResult::kPassed" \
  "FW-P13-001 must implement unexpected reboot and simulated power-loss resilience checks." \
  "$FIRMWARE_DIR/test/integration/test_resilience_boot_power_integration.cpp"

check_present \
  "test_resilience_boot_power_integration.cpp|test_resilience_boot_power_integration" \
  "Phase 13 integration runner must execute the boot and power resilience checks." \
  "$FIRMWARE_DIR/test/integration/run_phase13_integration_tests.sh"

check_present \
  "kWifiLoss|kMqttLoss|WIFI_OFFLINE|MQTT_OFFLINE|MqttReconnected|requestMode" \
  "FW-P13-002 must implement Wi-Fi and MQTT resilience checks without blocking local automation." \
  "$FIRMWARE_DIR/test/integration/test_resilience_network_mqtt_integration.cpp"

check_present \
  "test_resilience_network_mqtt_integration.cpp|test_resilience_network_mqtt_integration" \
  "Phase 13 integration runner must execute the Wi-Fi and MQTT resilience checks." \
  "$FIRMWARE_DIR/test/integration/run_phase13_integration_tests.sh"

check_present \
  "kTemperatureSensorOffline|kWaterLevelSensorOffline|TEMPERATURE_SENSOR_OFFLINE|ATO_SENSOR_OFFLINE|TemperatureSensorRecovered|WaterLevelSensorRecovered" \
  "FW-P13-003 must implement sensor-offline resilience checks without unintended actuator commands." \
  "$FIRMWARE_DIR/test/integration/test_resilience_sensor_offline_integration.cpp"

check_present \
  "test_resilience_sensor_offline_integration.cpp|test_resilience_sensor_offline_integration" \
  "Phase 13 integration runner must execute the sensor-offline resilience checks." \
  "$FIRMWARE_DIR/test/integration/run_phase13_integration_tests.sh"

check_present \
  "kAtoStuck|ATO_TIMEOUT|ATO_RECOVERED|FEEDING|TPA|MAINTENANCE|wifiConnected|mqttConnected" \
  "FW-P13-004 must implement ATO stuck, timeout, cooldown, mode gate, and connectivity-independent fail-safe checks." \
  "$FIRMWARE_DIR/test/integration/test_resilience_ato_stuck_integration.cpp"

check_present \
  "test_resilience_ato_stuck_integration.cpp|test_resilience_ato_stuck_integration" \
  "Phase 13 integration runner must execute the ATO stuck resilience checks." \
  "$FIRMWARE_DIR/test/integration/run_phase13_integration_tests.sh"

check_present \
  "buildResilienceFinalReport|Phase 13 resilience final report|validation-limitations|Pendente para Hardware Validation" \
  "FW-P13-005 must provide a deterministic final resilience report with no-hardware limitations." \
  "$FIRMWARE_DIR/include/resilience/resilience_report.h" \
  "$FIRMWARE_DIR/src/resilience/resilience_report.cpp" \
  "$FIRMWARE_DIR/test/integration/test_resilience_end_to_end_smoke.cpp"

check_present \
  "assertFinalReportCoversCanonicalScenarios|assertFinalReportFailsWhenScenarioMissingOrFailed|scenarios=7 passed=7 failed=0 not-run=0" \
  "FW-P13-005 smoke must fail on missing or failed canonical scenarios and cover all Phase 13 scenarios." \
  "$FIRMWARE_DIR/test/integration/test_resilience_end_to_end_smoke.cpp"

check_present \
  "run_phase13_integration_tests.sh|run_phase13_scope_review.sh|pio run|Pendente para Hardware Validation" \
  "FW-P13-005 must expose the consolidated Phase 13 validation runner and pending hardware label." \
  "$FIRMWARE_DIR/test/run_phase13_validation.sh"

check_absent \
  "fakes/fake_|test/fakes|FakePowerCycleContext|FakeResilience" \
  "Real firmware code must not depend on resilience fakes." \
  "$FIRMWARE_DIR/include/resilience" \
  "$FIRMWARE_DIR/src/resilience"

check_absent \
  "Supabase|Edge Functions|dashboard|push notification|React Native|mobile app|cloud sync|historico remoto|histórico remoto|UI|OTA|BLE|provisioning" \
  "FW-P13-000 must not implement cloud, app, UI, remote history, OTA, BLE, or provisioning." \
  "$FIRMWARE_DIR/include/resilience" \
  "$FIRMWARE_DIR/src/resilience" \
  "$FIRMWARE_DIR/test/integration/test_resilience_end_to_end_smoke.cpp"

check_absent \
  "ESP32 real|sensor real|sensores reais|relay real|relays reais|bomba real|luminaria real|luminária real|roteador real|broker real|Mosquitto real|Serial real|aquario|aquário|bancada|teste fisico|teste físico" \
  "FW-P13-000 checks must not require physical hardware, real network, real broker, aquarium, or bench validation." \
  "$FIRMWARE_DIR/test/integration/run_phase13_integration_tests.sh" \
  "$FIRMWARE_DIR/test/integration/test_resilience_end_to_end_smoke.cpp" \
  "$FIRMWARE_DIR/test/integration/test_resilience_boot_power_integration.cpp" \
  "$FIRMWARE_DIR/test/integration/test_resilience_network_mqtt_integration.cpp" \
  "$FIRMWARE_DIR/test/integration/test_resilience_sensor_offline_integration.cpp" \
  "$FIRMWARE_DIR/test/integration/test_resilience_ato_stuck_integration.cpp"

if find "$FIRMWARE_DIR/test/unit" -type f \( -iname '*resilience*' -o -iname '*phase13*' \) -print | grep -q .; then
  echo "FW-P13-000 must not create resilience or phase13 unit tests." >&2
  find "$FIRMWARE_DIR/test/unit" -type f \( -iname '*resilience*' -o -iname '*phase13*' \) -print >&2
  exit 1
fi

if ! git -C "$REPO_DIR" diff --quiet -- architecture specs tasks/firmware/firmware-master-plan.md; then
  echo "FW-P13-000 must not alter architecture, specs, or firmware master plan." >&2
  git -C "$REPO_DIR" diff --name-only -- architecture specs tasks/firmware/firmware-master-plan.md >&2
  exit 1
fi

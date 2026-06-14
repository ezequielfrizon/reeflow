#!/usr/bin/env bash
set -euo pipefail

FIRMWARE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
REPO_DIR="$(cd "$FIRMWARE_DIR/.." && pwd)"

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

check_present() {
  local pattern="$1"
  local message="$2"
  shift 2

  if ! grep -RInE "$pattern" "$@" >/dev/null; then
    echo "$message" >&2
    exit 1
  fi
}

check_absent \
  "test/fakes|fakes/" \
  "Real firmware code must not include test fakes or mocks." \
  "$FIRMWARE_DIR/include" \
  "$FIRMWARE_DIR/src"

check_present \
  "network-wifi|network-status|network-ntp|network-heartbeat" \
  "Phase 10 must register network tasks in the app scheduler." \
  "$FIRMWARE_DIR/src/app/core_app.cpp"

check_present \
  "restoreWifi" \
  "Phase 10 boot integration must preserve Wi-Fi restore before network runtime." \
  "$FIRMWARE_DIR/src/app/core_app.cpp"

check_present \
  "ArduinoWifiAdapter|SntpClient|WifiService|NtpService|NetworkStatusService|NetworkHeartbeat" \
  "Phase 10 default app must wire real network services behind boundaries." \
  "$FIRMWARE_DIR/src/app/core_app_arduino.cpp"

check_present \
  "FakeWifiAdapter|FakeNtpClient|FakeInternetProbe" \
  "Phase 10 host integration tests must use fakes for network dependencies." \
  "$FIRMWARE_DIR/test/integration/test_network_boot_scheduler_integration.cpp"

check_absent \
  "PubSubClient|MQTTClient|connectMqtt\\(|mqttConnect\\(|mqtt\\.connect|publishTelemetry|mqttSubscribe\\(|subscribeMqtt\\(|Mosquitto|broker|AlertManager|Alert Manager|Supabase|cloud|push|notification|dashboard|remote command|comando remoto|BLE|captive portal" \
  "Phase 10 must not initialize MQTT, broker, cloud, app, Alert Manager, provisioning, notifications, dashboard, or remote commands." \
  "$FIRMWARE_DIR/include/network" \
  "$FIRMWARE_DIR/src/network" \
  "$FIRMWARE_DIR/src/app/core_app.h" \
  "$FIRMWARE_DIR/src/app/core_app.cpp" \
  "$FIRMWARE_DIR/src/app/core_app_arduino.cpp" \
  "$FIRMWARE_DIR/test/integration/test_network_boot_scheduler_integration.cpp"

check_absent \
  "WiFi\\.begin|configTime|Serial\\.(read|parse|available)|access point real|Internet real|NTP real|ESP32 conectado|teste fisico|teste físico" \
  "Phase 10 host integration tests must not depend on real Wi-Fi, NTP, ESP32, Serial, or physical validation." \
  "$FIRMWARE_DIR/test/integration/test_network_boot_scheduler_integration.cpp" \
  "$FIRMWARE_DIR/test/integration/run_phase10_integration_tests.sh"

if ! git -C "$REPO_DIR" diff --quiet -- architecture specs tasks; then
  echo "Phase 10 must not alter architecture, specs, or task documents." >&2
  git -C "$REPO_DIR" diff --name-only -- architecture specs tasks >&2
  exit 1
fi

echo "Pendente para Hardware Validation: roteador real, DHCP real, RSSI real, perda real de sinal, NTP publico, ESP32 real, antena, caixa eletrica, aquario e bancada real."

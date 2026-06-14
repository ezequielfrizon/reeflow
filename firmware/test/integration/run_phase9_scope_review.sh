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
  "PreferencesStorageBackend" \
  "Phase 9 must compile the real NVS Preferences storage backend." \
  "$FIRMWARE_DIR/src/storage" \
  "$FIRMWARE_DIR/src/app"

check_present \
  "StorageWritePolicy|markChanged|flushAllDirty|runStorageFlushTask" \
  "Phase 9 must include controlled dirty-domain flush behavior." \
  "$FIRMWARE_DIR/include/storage" \
  "$FIRMWARE_DIR/src/storage" \
  "$FIRMWARE_DIR/src/app"

check_absent \
  "WiFi\\.begin|WiFi\\.mode|WiFi\\.reconnect|WiFiClient|PubSubClient|MQTTClient|connectMqtt|mqttConnect|mqtt\\.connect|publishTelemetry|mqttSubscribe|subscribeMqtt|NTP|configTime|AlertManager|Alert Manager|Supabase|cloud|push|notification|history|historico|histórico|remote command|comando remoto|Serial\\.(read|parse|available)" \
  "Phase 9 must not implement Wi-Fi runtime, MQTT runtime, NTP, Alert Manager, cloud, app, history, notifications, remote commands, or Serial command handling." \
  "$FIRMWARE_DIR/include/storage" \
  "$FIRMWARE_DIR/src/storage" \
  "$FIRMWARE_DIR/src/app/core_app.h" \
  "$FIRMWARE_DIR/src/app/core_app.cpp" \
  "$FIRMWARE_DIR/src/app/core_app_arduino.cpp"

check_absent \
  "currentTemperature|currentLevel|relays\\.|currentPWM|pumpRunning|wifiConnected|mqttConnected|activeAlerts|systemHealth|uptime|freeHeap|cpuUsage" \
  "Phase 9 storage must not persist volatile runtime readings or hardware-derived state." \
  "$FIRMWARE_DIR/include/storage" \
  "$FIRMWARE_DIR/src/storage"

check_absent \
  "updateRelaysState|setLocalRelay|writeDuty|setRelay\\(|allOff\\(|WiFi\\.begin|mqtt\\.connect" \
  "Phase 9 boot restore must not become physical relay/PWM/network actuation." \
  "$FIRMWARE_DIR/include/storage" \
  "$FIRMWARE_DIR/src/storage"

check_present \
  "FakeStorageBackend" \
  "Phase 9 storage validation must use fake storage in tests." \
  "$FIRMWARE_DIR/test/fakes" \
  "$FIRMWARE_DIR/test/unit" \
  "$FIRMWARE_DIR/test/integration"

check_absent \
  "PreferencesStorageBackend|<Preferences\\.h>" \
  "Phase 9 host tests must not require real NVS Preferences." \
  "$FIRMWARE_DIR/test/fakes" \
  "$FIRMWARE_DIR/test/unit" \
  "$FIRMWARE_DIR/test/integration/test_storage_boot_restore_integration.cpp" \
  "$FIRMWARE_DIR/test/integration/test_storage_scheduler_integration.cpp" \
  "$FIRMWARE_DIR/test/integration/run_phase9_storage_boot_restore_integration_tests.sh" \
  "$FIRMWARE_DIR/test/integration/run_phase9_storage_scheduler_integration_tests.sh"

if ! git -C "$REPO_DIR" diff --quiet -- architecture specs tasks; then
  echo "Phase 9 must not alter architecture, specs, or task documents." >&2
  git -C "$REPO_DIR" diff --name-only -- architecture specs tasks >&2
  exit 1
fi

echo "Pendente para Hardware Validation: ESP32 real, NVS fisica, reboot fisico, ciclo de energia, flash real, Wi-Fi real, broker MQTT real, sensores, reles, luminaria, Serial real, aquario e seguranca de bancada."

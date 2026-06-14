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
  "delay[[:space:]]*\\(|while[[:space:]]*\\([[:space:]]*(true|1)[[:space:]]*\\)|for[[:space:]]*\\([^;]*;[[:space:]]*;|busy wait" \
  "Phase 11 MQTT services must not block the scheduler with delay or unbounded loops." \
  "$FIRMWARE_DIR/include/mqtt" \
  "$FIRMWARE_DIR/src/mqtt"

check_absent \
  "Preferences|NVS|StorageService|StorageBackend|storage backend|arquivo|file system|filesystem" \
  "Phase 11 MQTT outbox must remain volatile and must not use persistent storage." \
  "$FIRMWARE_DIR/include/mqtt" \
  "$FIRMWARE_DIR/src/mqtt"

check_absent \
  "Supabase|Cloud|cloud bridge|mobile|dashboard|AlertManager|Alert Manager|push notification|OTA|BLE|captive portal|resilien|historico remoto|histórico remoto" \
  "Phase 11 must not implement cloud, mobile, Alert Manager, resilience phase, provisioning, OTA, or remote history." \
  "$FIRMWARE_DIR/include/mqtt" \
  "$FIRMWARE_DIR/src/mqtt" \
  "$FIRMWARE_DIR/src/app/core_app.h" \
  "$FIRMWARE_DIR/src/app/core_app.cpp" \
  "$FIRMWARE_DIR/src/app/core_app_arduino.cpp"

check_absent \
  "WiFi\\.begin|Serial\\.(read|parse|available)|Mosquitto real|broker real|ESP32 real|monitor Serial real|aquario|aquário|teste fisico|teste físico" \
  "Phase 11 automated checks must not require physical MQTT, ESP32, Serial, or aquarium validation." \
  "$FIRMWARE_DIR/test/integration/run_phase11_integration_tests.sh"

check_present \
  "Pendente para Hardware Validation" \
  "Phase 11 validation must register real-world MQTT checks as pending hardware validation." \
  "$FIRMWARE_DIR/test/run_phase11_validation.sh"

if find "$FIRMWARE_DIR/test/unit" -type f \( -iname '*mqtt*' -o -iname '*phase11*' \) -print | grep -q .; then
  echo "Phase 11 must not create MQTT or phase11 unit tests." >&2
  find "$FIRMWARE_DIR/test/unit" -type f \( -iname '*mqtt*' -o -iname '*phase11*' \) -print >&2
  exit 1
fi

if ! git -C "$REPO_DIR" diff --quiet -- architecture specs tasks/firmware/firmware-master-plan.md; then
  echo "Phase 11 must not alter architecture, specs, or firmware master plan." >&2
  git -C "$REPO_DIR" diff --name-only -- architecture specs tasks/firmware/firmware-master-plan.md >&2
  exit 1
fi

echo "Pendente para Hardware Validation: broker Mosquitto real, rede local real, ESP32 real, monitor Serial real, credenciais reais, queda real de Wi-Fi, latencia real, antena, caixa eletrica, aquario e bancada real."

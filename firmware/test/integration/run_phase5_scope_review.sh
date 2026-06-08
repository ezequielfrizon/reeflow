#!/usr/bin/env bash
set -euo pipefail

FIRMWARE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

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

check_absent \
  "diagnostic|diagnostics/" \
  "Phase 5 functional relay code must not depend on Phase 1 diagnostics." \
  "$FIRMWARE_DIR/include/modules/relays" \
  "$FIRMWARE_DIR/src/modules/relays" \
  "$FIRMWARE_DIR/src/drivers/relays/gpio_relay_controller.h" \
  "$FIRMWARE_DIR/src/drivers/relays/gpio_relay_controller.cpp"

check_absent \
  "test/fakes|fakes/" \
  "Real firmware code must not include test fakes or mocks." \
  "$FIRMWARE_DIR/include" \
  "$FIRMWARE_DIR/src"

check_absent \
  "ATO_START|ATO_STOP|ATO_TIMEOUT|ATO_SENSOR_OFFLINE|ATO_RECOVERED" \
  "Phase 5 must not create ATO events." \
  "$FIRMWARE_DIR/include/modules/relays" \
  "$FIRMWARE_DIR/src/modules/relays" \
  "$FIRMWARE_DIR/src/drivers/relays/gpio_relay_controller.h" \
  "$FIRMWARE_DIR/src/drivers/relays/gpio_relay_controller.cpp" \
  "$FIRMWARE_DIR/src/app/core_app.h" \
  "$FIRMWARE_DIR/src/app/core_app.cpp" \
  "$FIRMWARE_DIR/src/app/core_app_arduino.cpp"

check_absent \
  "waterLevel|WaterLevel|minimumLevel|maximumLevel|LOW_LEVEL|REFILLING|TIMEOUT|cooldown|Cooldown" \
  "Phase 5 relay module must not use water level or implement ATO automation." \
  "$FIRMWARE_DIR/include/modules/relays" \
  "$FIRMWARE_DIR/src/modules/relays" \
  "$FIRMWARE_DIR/src/drivers/relays/gpio_relay_controller.h" \
  "$FIRMWARE_DIR/src/drivers/relays/gpio_relay_controller.cpp"

check_absent \
  "MQTT|mqtt|WiFi|NVS|Preferences|AlertManager|Alert Manager|cloud|Cloud|OperationalMode|MODE_" \
  "Phase 5 must not initialize future-phase capabilities." \
  "$FIRMWARE_DIR/include/modules/relays" \
  "$FIRMWARE_DIR/src/modules/relays" \
  "$FIRMWARE_DIR/src/drivers/relays/gpio_relay_controller.h" \
  "$FIRMWARE_DIR/src/drivers/relays/gpio_relay_controller.cpp" \
  "$FIRMWARE_DIR/src/app/core_app.h" \
  "$FIRMWARE_DIR/src/app/core_app.cpp" \
  "$FIRMWARE_DIR/src/app/core_app_arduino.cpp"

check_absent \
  "registerTask\\([^)]*relay|registerTask\\([^)]*Relay" \
  "Phase 5 must not register a periodic relay task." \
  "$FIRMWARE_DIR/src/app/core_app.cpp"

check_absent \
  "RelaySource::kMqtt|RelaySource::kApp|RelaySource::kAutomation" \
  "Phase 5 runtime must not implement MQTT, APP, or AUTOMATION relay command sources." \
  "$FIRMWARE_DIR/src/modules/relays" \
  "$FIRMWARE_DIR/src/app/core_app.h" \
  "$FIRMWARE_DIR/src/app/core_app.cpp" \
  "$FIRMWARE_DIR/src/app/core_app_arduino.cpp"

echo "Pendente para Hardware Validation: modulo de rele real, carga AC/DC real, bomba, aquecedor, tomada, medicao eletrica, resposta fisica, ruido e seguranca de bancada."

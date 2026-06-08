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

check_path_absent() {
  local path="$1"
  local message="$2"

  if [ -e "$path" ]; then
    echo "$message" >&2
    find "$path" -maxdepth 3 -print >&2
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

check_absent \
  "Preferences|NVS|MQTT|WiFi|wifi|AlertManager|Alert Manager|Cloud|cloud|push|notification|history|historico|histórico|remote|remoto|comando remoto|Serial" \
  "Phase 8 must not implement NVS, Wi-Fi, MQTT, Alert Manager, cloud, app, history, notifications, remote commands, or Serial commands." \
  "$FIRMWARE_DIR/include/modules/lighting" \
  "$FIRMWARE_DIR/src/modules/lighting" \
  "$FIRMWARE_DIR/src/drivers/lighting" \
  "$FIRMWARE_DIR/src/app/core_app.h" \
  "$FIRMWARE_DIR/src/app/core_app.cpp" \
  "$FIRMWARE_DIR/src/app/core_app_arduino.cpp"

check_absent \
  "lighting_store|LightingStore|Preferences|NVS|Repository|repository|persist|Persist" \
  "Phase 8 must not implement lighting storage, NVS backend, repository, or persistence." \
  "$FIRMWARE_DIR/include/modules/lighting" \
  "$FIRMWARE_DIR/src/modules/lighting" \
  "$FIRMWARE_DIR/src/drivers/lighting"

check_absent \
  "updateTemperatureState|updateWaterLevelState|updateRelaysState|updateModesState|updateAtoState|updateNetworkState|updateAlertsState|updateSystemHealthState" \
  "Lighting module must not mutate unrelated System State blocks." \
  "$FIRMWARE_DIR/include/modules/lighting" \
  "$FIRMWARE_DIR/src/modules/lighting"

check_absent \
  "ModeService|ModeAutomationGate|RelayModeEffects|AtoService|RelayService" \
  "Lighting module must not depend on modes, ATO, or relay services." \
  "$FIRMWARE_DIR/include/modules/lighting" \
  "$FIRMWARE_DIR/src/modules/lighting" \
  "$FIRMWARE_DIR/src/drivers/lighting"

check_absent \
  "Lighting|lighting|LIGHTING|PWM|sunrise|sunset|moonlight|acclimation|Aclim" \
  "Modes, ATO, and relay modules must not implement lighting behavior." \
  "$FIRMWARE_DIR/include/modules/modes" \
  "$FIRMWARE_DIR/src/modules/modes" \
  "$FIRMWARE_DIR/include/modules/ato" \
  "$FIRMWARE_DIR/src/modules/ato" \
  "$FIRMWARE_DIR/include/modules/relays" \
  "$FIRMWARE_DIR/src/modules/relays"

check_absent \
  "GPIO13|gpio13|kGpio13|PWM Reserva|Reserva.*PWM|gpio[[:space:]]*=[[:space:]]*13|gpio[[:space:]]*==[[:space:]]*13" \
  "Phase 8 lighting code must not expose GPIO13 as a functional channel." \
  "$FIRMWARE_DIR/include/modules/lighting" \
  "$FIRMWARE_DIR/src/modules/lighting" \
  "$FIRMWARE_DIR/src/drivers/lighting"

check_present \
  "PWM Branco|PWM Azul|PWM Royal Blue|PWM Moonlight/UV" \
  "Lighting PWM controller must use official hardware pin contract names." \
  "$FIRMWARE_DIR/src/drivers/lighting"

check_path_absent \
  "$FIRMWARE_DIR/src/modules/lighting_store" \
  "Phase 8 must not create lighting_store."

check_path_absent \
  "$FIRMWARE_DIR/include/modules/lighting_store" \
  "Phase 8 must not create lighting_store includes."

if ! git -C "$REPO_DIR" diff --quiet -- architecture specs tasks/firmware-master-plan.md tasks/firmware-phase-08-light-system.md; then
  echo "Phase 8 must not alter architecture, specs, or task documents." >&2
  git -C "$REPO_DIR" diff --name-only -- architecture specs tasks/firmware-master-plan.md tasks/firmware-phase-08-light-system.md >&2
  exit 1
fi

echo "Pendente para Hardware Validation: luminaria real, MOSFET real, fonte 12V, carga LED, corrente, dissipacao, resposta visual, medicao de PWM, ruido eletrico, Serial real e seguranca de bancada."

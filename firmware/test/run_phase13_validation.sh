#!/usr/bin/env bash
set -euo pipefail

FIRMWARE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TEST_DIR="$FIRMWARE_DIR/test"

bash "$TEST_DIR/integration/run_phase13_integration_tests.sh"
bash "$TEST_DIR/integration/run_phase13_scope_review.sh"

pio run -d "$FIRMWARE_DIR"

echo "Pendente para Hardware Validation: reboot fisico real, queda real de energia, ESP32 real, sensores reais, relays reais, bomba ATO real, luminaria real, roteador real, Internet real, broker Mosquitto real, monitor Serial real, caixa eletrica, bancada e aquario."

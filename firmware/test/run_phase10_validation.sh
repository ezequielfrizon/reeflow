#!/usr/bin/env bash
set -euo pipefail

FIRMWARE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TEST_DIR="$FIRMWARE_DIR/test"

bash "$TEST_DIR/unit/run_phase10_unit_tests.sh"
bash "$TEST_DIR/integration/run_phase10_integration_tests.sh"
bash "$TEST_DIR/integration/run_phase10_scope_review.sh"

pio run -d "$FIRMWARE_DIR"

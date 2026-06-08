#!/usr/bin/env bash
set -euo pipefail

FIRMWARE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${TMPDIR:-/tmp}/reeflow_phase6_ato_contracts_tests"
mkdir -p "$BUILD_DIR"

g++ -std=c++17 -Wall -Wextra -Werror \
  -I"$FIRMWARE_DIR/include" \
  -I"$FIRMWARE_DIR/src" \
  -I"$FIRMWARE_DIR/test" \
  "$FIRMWARE_DIR/test/unit/test_ato_contracts.cpp" \
  "$FIRMWARE_DIR/src/modules/ato/ato_events.cpp" \
  -o "$BUILD_DIR/test_ato_contracts"

"$BUILD_DIR/test_ato_contracts"

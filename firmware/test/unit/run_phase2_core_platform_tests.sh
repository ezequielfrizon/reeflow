#!/usr/bin/env bash
set -euo pipefail

FIRMWARE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${TMPDIR:-/tmp}/reeflow_phase2_core_platform_tests"
mkdir -p "$BUILD_DIR"

g++ -std=c++17 -Wall -Wextra -Werror \
  -I"$FIRMWARE_DIR/include" \
  -I"$FIRMWARE_DIR/src" \
  -I"$FIRMWARE_DIR/test" \
  "$FIRMWARE_DIR/test/unit/test_core_platform_boundaries.cpp" \
  "$FIRMWARE_DIR/src/core/platform/core_platform.cpp" \
  -o "$BUILD_DIR/test_core_platform_boundaries"

"$BUILD_DIR/test_core_platform_boundaries"

#!/usr/bin/env bash
set -euo pipefail

TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

bash "$TEST_DIR/run_phase9_storage_boot_restore_integration_tests.sh"
bash "$TEST_DIR/run_phase9_storage_scheduler_integration_tests.sh"

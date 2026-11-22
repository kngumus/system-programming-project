
#!/usr/bin/env bash

set -euo pipefail

# Run from the project root
cd "$(dirname "$0")/.."

# Create a temporary file for watchdog output
OUTPUT=$(mktemp)

# Clean up even if the test fails
cleanup() {
    if [[ -n "${WATCHDOG_PID:-}" ]]; then
        kill -TERM "$WATCHDOG_PID" 2>/dev/null || true
        wait "$WATCHDOG_PID" 2>/dev/null || true
    fi
    rm -f "$OUTPUT"
}
trap cleanup EXIT

echo "Running Test 1: Normal Startup..."

# Start watchdog in the background
./watchdog > "$OUTPUT" 2>&1 &
WATCHDOG_PID=$!

# Give the workers time to start
sleep 2

# Verify watchdog is still running
if ! kill -0 "$WATCHDOG_PID" 2>/dev/null; then
    echo "FAIL: Watchdog terminated unexpectedly"
    exit 1
fi

# Verify both workers started
if grep -q "Started worker 1 with PID" "$OUTPUT" &&
   grep -q "Started worker 2 with PID" "$OUTPUT"; then
    echo "PASS: Both workers started successfully"
else
    echo "FAIL: One or both workers did not start"
    cat "$OUTPUT"
    exit 1
fi

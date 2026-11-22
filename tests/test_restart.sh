
#!/usr/bin/env bash

set -euo pipefail

cd "$(dirname "$0")/.."

OUTPUT=$(mktemp)
WATCHDOG_PID=""

cleanup() {
    if [[ -n "$WATCHDOG_PID" ]]; then
        kill -TERM "$WATCHDOG_PID" 2>/dev/null || true
        wait "$WATCHDOG_PID" 2>/dev/null || true
    fi
    rm -f "$OUTPUT"
}
trap cleanup EXIT

echo "Running Test 2: Automatic Worker Restart..."

# Start watchdog
./watchdog > "$OUTPUT" 2>&1 &
WATCHDOG_PID=$!

sleep 2

# Find worker 1's PID from watchdog output
WORKER_PID=$(sed -n 's/.*Started worker 1 with PID \([0-9]*\).*/\1/p' "$OUTPUT" | head -n 1)

if [[ -z "$WORKER_PID" ]]; then
    echo "FAIL: Worker 1 was not started"
    cat "$OUTPUT"
    exit 1
fi

echo "Worker 1 PID: $WORKER_PID"

# Simulate an unexpected worker crash
kill -KILL "$WORKER_PID"

# Allow watchdog to detect and restart worker
sleep 2

# Check that worker 1 was started again
RESTART_COUNT=$(grep -c "Started worker 1 with PID" "$OUTPUT" || true)

if [[ "$RESTART_COUNT" -ge 2 ]]; then
    echo "PASS: Worker 1 restarted automatically"
else
    echo "FAIL: Worker 1 was not restarted"
    cat "$OUTPUT"
    exit 1
fi

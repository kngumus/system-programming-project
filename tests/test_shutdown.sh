
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

echo "Running Test 5: Graceful Shutdown..."

./watchdog > "$OUTPUT" 2>&1 &
WATCHDOG_PID=$!

sleep 2

# Capture the PIDs of both workers
WORKER1_PID=$(sed -n 's/.*Started worker 1 with PID \([0-9]*\).*/\1/p' "$OUTPUT" | head -n 1)
WORKER2_PID=$(sed -n 's/.*Started worker 2 with PID \([0-9]*\).*/\1/p' "$OUTPUT" | head -n 1)

if [[ -z "$WORKER1_PID" || -z "$WORKER2_PID" ]]; then
    echo "FAIL: Could not identify both workers"
    cat "$OUTPUT"
    exit 1
fi

# Request graceful shutdown
kill -TERM "$WATCHDOG_PID"

# Wait up to 8 seconds for watchdog to exit
FINISHED=0

for i in {1..80}; do
    if [[ ! -d "/proc/$WATCHDOG_PID" ]]; then
        FINISHED=1
        break
    fi
    sleep 0.1
done

if [[ "$FINISHED" -ne 1 ]]; then
    echo "FAIL: Watchdog did not shut down in time"
    cat "$OUTPUT"
    exit 1
fi

# Check that workers no longer exist
if kill -0 "$WORKER1_PID" 2>/dev/null ||
   kill -0 "$WORKER2_PID" 2>/dev/null; then
    echo "FAIL: Worker process still exists after shutdown"
    cat "$OUTPUT"
    exit 1
fi

echo "PASS: Watchdog and both workers shut down successfully"


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

echo "Running Test 4: SIGUSR1 Status Report..."

./watchdog > "$OUTPUT" 2>&1 &
WATCHDOG_PID=$!

sleep 2

if ! kill -0 "$WATCHDOG_PID" 2>/dev/null; then
    echo "FAIL: Watchdog is not running"
    cat "$OUTPUT"
    exit 1
fi

# Request worker status information
kill -USR1 "$WATCHDOG_PID"

sleep 1


# Verify the status report contains both workers
if grep -q -- "--- Worker Status ---" "$OUTPUT" &&
   grep -Eq "Worker 1: PID [0-9]+, RUNNING, Restarts: 0/3" "$OUTPUT" &&
   grep -Eq "Worker 2: PID [0-9]+, RUNNING, Restarts: 0/3" "$OUTPUT"; then

    echo "PASS: SIGUSR1 status report generated correctly"

else
    echo "FAIL: SIGUSR1 status report is missing or incorrect"
    cat "$OUTPUT"
    exit 1
fi

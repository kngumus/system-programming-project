
#!/usr/bin/env bash

set -euo pipefail

cd "$(dirname "$0")/.."

OUTPUT=$(mktemp)
LOG_FILE="watchdog.log"
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

# Remove previous log entries so this test checks only fresh data.
rm -f "$LOG_FILE"

./watchdog > "$OUTPUT" 2>&1 &
WATCHDOG_PID=$!

sleep 2

if ! kill -0 "$WATCHDOG_PID" 2>/dev/null; then
    echo "FAIL: Watchdog is not running"
    cat "$OUTPUT"
    exit 1
fi

# Request worker status information.
kill -USR1 "$WATCHDOG_PID"

sleep 1

# Verify terminal output.
if ! grep -q -- "--- Worker Status ---" "$OUTPUT" ||
   ! grep -Eq "Worker 1: PID [0-9]+, RUNNING, Restarts: 0/3" "$OUTPUT" ||
   ! grep -Eq "Worker 2: PID [0-9]+, RUNNING, Restarts: 0/3" "$OUTPUT"; then

    echo "FAIL: SIGUSR1 terminal status report is incorrect"
    cat "$OUTPUT"
    exit 1
fi

# Verify log file exists.
if [[ ! -f "$LOG_FILE" ]]; then
    echo "FAIL: watchdog.log was not created"
    exit 1
fi

# Verify the SIGUSR1 request and both worker statuses are logged.
if ! grep -q "Worker status report requested" "$LOG_FILE" ||
   ! grep -Eq "Worker 1: PID [0-9]+, RUNNING, Restarts: 0/3" "$LOG_FILE" ||
   ! grep -Eq "Worker 2: PID [0-9]+, RUNNING, Restarts: 0/3" "$LOG_FILE"; then

    echo "FAIL: SIGUSR1 status report is missing from watchdog.log"
    cat "$LOG_FILE"
    exit 1
fi

echo "PASS: SIGUSR1 status report verified in terminal and log file"

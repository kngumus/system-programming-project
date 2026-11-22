
#!/usr/bin/env bash

set -euo pipefail

cd "$(dirname "$0")/.."

echo "Running Test 7: Normal Child Termination..."

# Ensure the production watchdog executable exists.
make watchdog >/dev/null

# Use a separate directory to avoid modifying the real worker.
TEST_DIR=$(mktemp -d)
WATCHDOG_PID=""

cleanup() {
    if [[ -n "$WATCHDOG_PID" ]]; then
        kill -TERM "$WATCHDOG_PID" 2>/dev/null || true
        wait "$WATCHDOG_PID" 2>/dev/null || true
    fi

    rm -rf "$TEST_DIR"
}
trap cleanup EXIT

cp ./watchdog "$TEST_DIR/watchdog"

# Create a worker that exits normally.
cat > "$TEST_DIR/normal_worker.c" <<'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    printf("Test worker exiting normally (PID: %ld)\n",
           (long)getpid());

    fflush(stdout);
    sleep(1);
    return EXIT_SUCCESS;
}
EOF

gcc -Wall -Wextra -std=c11 \
    -o "$TEST_DIR/worker" "$TEST_DIR/normal_worker.c"

# The watchdog expects ./worker in its current directory.
(
    cd "$TEST_DIR"
    exec ./watchdog > output.txt 2>&1
) &
WATCHDOG_PID=$!

sleep 2

OUTPUT="$TEST_DIR/output.txt"
LOG_FILE="$TEST_DIR/watchdog.log"


# Verify normal termination using the log file.
if [[ ! -f "$LOG_FILE" ]] ||
   ! grep -q "Worker 1 exited with code 0" "$LOG_FILE" ||
   ! grep -q "Worker 2 exited with code 0" "$LOG_FILE" ||
   ! grep -q "Worker 1 completed normally" "$LOG_FILE" ||
   ! grep -q "Worker 2 completed normally" "$LOG_FILE"; then

    echo "FAIL: Normal child termination was not logged correctly"
    [[ -f "$LOG_FILE" ]] && cat "$LOG_FILE"
    exit 1
fi


# Neither worker should be restarted.
if grep -q "Restarting worker" "$OUTPUT" ||
   grep -q "Restarting worker" "$LOG_FILE"; then

    echo "FAIL: A normally terminated worker was restarted"
    cat "$OUTPUT"
    exit 1
fi

# Both workers must have started exactly once.
for worker_id in 1 2; do
    count=$(grep -c "Started worker $worker_id with PID" "$OUTPUT" || true)

    if [[ "$count" -ne 1 ]]; then
        echo "FAIL: Worker $worker_id started $count times"
        cat "$OUTPUT"
        exit 1
    fi
done

echo "PASS: Both workers exited normally without restart"

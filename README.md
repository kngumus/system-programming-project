# POSIX Watchdog — Process Supervisor

A lightweight process supervision system developed in C for the System Programming course.

The watchdog monitors worker processes, detects unexpected termination, automatically restarts failed workers within a defined limit, and supports signal-based monitoring, event logging, and automated testing.

## Features

- **Process Management:** Starts and supervises two worker processes.
- **Automatic Restart:** Detects terminated workers and attempts to restart them.
- **Restart Limit:** Allows a maximum of three restarts per worker.
- **Signal Handling:** Uses POSIX signals for process monitoring, status reporting, and graceful shutdown.
- **Periodic Monitoring:** Uses `setitimer()` and `SIGALRM` to trigger monitoring every second.
- **Event Logging:** Records important watchdog events with timestamps and severity levels.
- **Automated Testing:** Includes shell scripts to verify core watchdog functionality.

## Project Structure

```text
system-programming-project/
├── include/
│   ├── logger.h
│   ├── process_manager.h
│   └── signal_manager.h
├── src/
│   ├── logger.c
│   ├── main.c
│   ├── process_manager.c
│   └── signal_manager.c
├── workers/
│   └── worker.c
├── tests/
│   ├── run_all_tests.sh
│   ├── test_startup.sh
│   ├── test_restart.sh
│   ├── test_restart_limit.sh
│   ├── test_sigusr1.sh
│   ├── test_shutdown.sh
│   ├── test_periodic.sh
│   ├── test_normal_exit.sh
│   └── test_logging_restart_limit.sh
├── Makefile
└── README.md
```

## Requirements

- Linux (developed and tested on Ubuntu)
- GCC compiler
- GNU Make
- Bash

## Build and Run

Clone the repository and navigate to the project directory.

Compile the watchdog and worker executables:

```bash
make
```

Start the watchdog:

```bash
./watchdog
```

The watchdog starts two worker processes and supervises their execution.

Remove compiled executables:

```bash
make clean
```

## Signal Handling

The watchdog uses POSIX signal handlers registered with `sigaction()`.

| Signal | Purpose |
|---|---|
| `SIGCHLD` | Detect child process termination |
| `SIGUSR1` | Request worker status information |
| `SIGINT` | Request graceful shutdown |
| `SIGTERM` | Request graceful shutdown |
| `SIGALRM` | Trigger periodic worker monitoring |

Signal handlers update `volatile sig_atomic_t` flags. The main monitoring loop processes these flags and performs the corresponding operations.

### Request Worker Status

While the watchdog is running, send:

```bash
kill -USR1 <watchdog_pid>
```

Replace `<watchdog_pid>` with the watchdog's process ID.

### Stop the Watchdog

Use `Ctrl+C` in the watchdog terminal or send:

```bash
kill -TERM <watchdog_pid>
```

## Automatic Worker Restart

The watchdog monitors two worker processes.

When a worker terminates unexpectedly, the watchdog detects the event and attempts to restart the worker.

Each worker has a maximum restart limit of three attempts. Once this limit is reached, the watchdog does not restart that worker again.

## Periodic Monitoring

Periodic monitoring is implemented using `setitimer()` with `ITIMER_REAL`.

The timer generates a `SIGALRM` signal approximately once per second.

The signal handler sets a flag, allowing the main monitoring loop to perform worker checks outside the signal handler.

The timer is stopped during watchdog shutdown.

## Event Logging

The logging module records watchdog events in:

```text
watchdog.log
```

Log messages include timestamps and severity levels:

- `INFO`
- `WARNING`
- `ERROR`

Logged events include worker startup, termination, restart attempts, restart limit violations, and watchdog shutdown.

## Automated Tests

Run the complete automated test suite:

```bash
bash tests/run_all_tests.sh
```

The suite verifies:

1. Normal worker startup
2. Automatic worker restart
3. Maximum restart limit enforcement
4. SIGUSR1 status reporting
5. Graceful shutdown
6. Periodic SIGALRM monitoring
7. Normal child termination without automatic restart

The logging restart-limit test can also be executed separately:

```bash
bash tests/test_logging_restart_limit.sh
```

### Latest Test Results

The integrated project was compiled and tested successfully on Ubuntu.

```text
Passed:  7
Failed:  0
Skipped: 0
Total:   7

ALL TESTS PASSED!
```

## Team Members

- Zeynep Burhan
- Kevser Naz Gümüş

## Course

SWE301 — System Programming
Sakarya University — Software Engineering Department
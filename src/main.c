
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>

#include "signal_manager.h"

#define WORKER_COUNT 2
#define SHUTDOWN_TIMEOUT 5

static pid_t children[WORKER_COUNT] = {0};
static int active_workers = 0;

static int start_worker(int index)
{
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        char worker_id[16];

        snprintf(worker_id, sizeof(worker_id),
                 "%d", index + 1);

        execl("./worker", "worker",
              worker_id, (char *)NULL);

        perror("execl");
        _exit(127);
    }

    children[index] = pid;
    active_workers++;

    printf("Started worker %d with PID %ld\n",
           index + 1, (long)pid);

    return 0;
}

static void check_children(void)
{
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < WORKER_COUNT; i++) {
            if (children[i] == pid) {
                children[i] = 0;
                active_workers--;

                if (WIFEXITED(status)) {
                    printf("Worker %d exited with code %d\n",
                           i + 1, WEXITSTATUS(status));
                }
                else if (WIFSIGNALED(status)) {
                    printf("Worker %d killed by signal %d\n",
                           i + 1, WTERMSIG(status));
                }

                break;
            }
        }
    }

    if (pid == -1 && errno != ECHILD) {
        perror("waitpid");
    }
}

static void print_status(void)
{
    printf("\n--- Worker Status ---\n");

    for (int i = 0; i < WORKER_COUNT; i++) {
        if (children[i] > 0) {
            printf("Worker %d: PID %ld, RUNNING\n",
                   i + 1, (long)children[i]);
        }
        else {
            printf("Worker %d: NOT RUNNING\n", i + 1);
        }
    }

    printf("---------------------\n\n");
    fflush(stdout);
}

static void shutdown_workers(void)
{
    printf("\nShutdown requested.\n");

    for (int i = 0; i < WORKER_COUNT; i++) {
        if (children[i] > 0) {
            if (kill(children[i], SIGTERM) == -1 &&
                errno != ESRCH) {
                perror("kill SIGTERM");
            }
        }
    }

    time_t deadline = time(NULL) + SHUTDOWN_TIMEOUT;

    while (active_workers > 0 && time(NULL) < deadline) {
        check_children();

        if (active_workers == 0)
            break;

        struct timespec delay = {0, 100000000L};
        nanosleep(&delay, NULL);
    }

    if (active_workers > 0) {
        printf("Timeout: sending SIGKILL.\n");

        for (int i = 0; i < WORKER_COUNT; i++) {
            if (children[i] > 0) {
                if (kill(children[i], SIGKILL) == -1 &&
                    errno != ESRCH) {
                    perror("kill SIGKILL");
                }
            }
        }

        for (int i = 0; i < WORKER_COUNT; i++) {
            if (children[i] > 0) {
                int status;
                pid_t result;

                do {
                    result = waitpid(children[i], &status, 0);
                } while (result == -1 && errno == EINTR);

                if (result == children[i]) {
                    children[i] = 0;
                    active_workers--;
                }
                else if (result == -1) {
                    perror("waitpid shutdown");
                }
            }
        }
    }

    printf("Watchdog shutdown complete.\n");
}

int main(void)
{
    printf("Watchdog started. Parent PID: %ld\n",
           (long)getpid());

    if (setup_signal_handlers() == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < WORKER_COUNT; i++) {
        if (shutdown_requested || start_worker(i) == -1) {
            shutdown_requested = 1;
            break;
        }
    }

    while (!shutdown_requested) {
        if (sigchld_received) {
            sigchld_received = 0;
            check_children();
        }

        if (sigusr1_received) {
            sigusr1_received = 0;
            check_children();
            print_status();
        }

        /* Periodically check for missed child events */
        check_children();

        struct timespec delay = {0, 100000000L};
        nanosleep(&delay, NULL);
    }

    shutdown_workers();

    return EXIT_SUCCESS;
}


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>

#include "process_manager.h"
#include "signal_manager.h"


#define SHUTDOWN_TIMEOUT 5


/* Information about our workers */
static Worker workers[WORKER_COUNT] = {0};

/* Create one worker process */
static int start_worker(int index)
{
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        /* Child process */
        char worker_id[16];

        snprintf(worker_id, sizeof(worker_id),
                 "%d", index + 1);

        execl("./worker", "worker",
              worker_id, (char *)NULL);

        /* Only reached if exec fails */
        perror("execl");
        _exit(127);
    }

    /* Parent process */
    workers[index].pid = pid;
    workers[index].running = 1;

    printf("Started worker %d with PID %ld\n",
           index + 1, (long)pid);

    fflush(stdout);

    return 0;
}

/* Start all workers */
int start_all_workers(void)
{
    for (int i = 0; i < WORKER_COUNT; i++) {
        if (shutdown_requested)
            return -1;

        if (start_worker(i) == -1)
            return -1;
    }

    return 0;
}


/* Handle a worker that has terminated */
static void handle_terminated_child(pid_t pid, int status)
{
    for (int i = 0; i < WORKER_COUNT; i++) {

        /* Find which worker had this PID */
        if (!workers[i].running ||
            workers[i].pid != pid)
            continue;

        workers[i].running = 0;
        workers[i].pid = 0;

        int unexpected = 0;

        if (WIFEXITED(status)) {
            int code = WEXITSTATUS(status);

            printf("Worker %d exited with code %d\n",
                   i + 1, code);

            if (code != 0)
                unexpected = 1;
        }
        else if (WIFSIGNALED(status)) {
            printf("Worker %d killed by signal %d\n",
                   i + 1, WTERMSIG(status));

            unexpected = 1;
        }

        /* Never restart during shutdown */
        if (shutdown_requested)
            return;

        /* Normal exit: no restart needed */
        if (!unexpected) {
            printf("Worker %d completed normally.\n",
                   i + 1);
            return;
        }

        /* Check restart limit */
        if (workers[i].restart_count >= MAX_RESTARTS) {
            printf("Worker %d restart limit reached.\n",
                   i + 1);
            return;
        }

        /* Restart the failed worker */
        workers[i].restart_count++;

        printf("Restarting worker %d (attempt %d/%d)\n",
               i + 1,
               workers[i].restart_count,
               MAX_RESTARTS);

        if (start_worker(i) == -1) {
            fprintf(stderr,
                    "Failed to restart worker %d\n",
                    i + 1);
        }

        return;
    }
}



/* Collect terminated child processes */
void check_children(void)
{
    int status;
    pid_t pid;

    while (1) {
        pid = waitpid(-1, &status, WNOHANG);

        if (pid > 0) {
            handle_terminated_child(pid, status);
            continue;
        }

        if (pid == 0)
            break;

        if (errno == EINTR)
            continue;

        if (errno != ECHILD)
            perror("waitpid");

        break;
    }
}



/* Display the current status of all workers */
void print_worker_status(void)
{
    printf("\n--- Worker Status ---\n");

    for (int i = 0; i < WORKER_COUNT; i++) {
        if (workers[i].running) {
            printf("Worker %d: PID %ld, RUNNING, "
                   "Restarts: %d/%d\n",
                   i + 1,
                   (long)workers[i].pid,
                   workers[i].restart_count,
                   MAX_RESTARTS);
        }
        else {
            printf("Worker %d: NOT RUNNING, "
                   "Restarts: %d/%d\n",
                   i + 1,
                   workers[i].restart_count,
                   MAX_RESTARTS);
        }
    }

    printf("---------------------\n\n");
    fflush(stdout);
}





/* Check whether any workers are still running */
static int any_worker_running(void)
{
    for (int i = 0; i < WORKER_COUNT; i++) {
        if (workers[i].running)
            return 1;
    }

    return 0;
}

/* Gracefully terminate all remaining workers */
void shutdown_workers(void)
{
    printf("\nShutdown requested.\n");

    /* Prevent automatic restarts during shutdown */
    shutdown_requested = 1;

    /* First, request graceful termination */
    for (int i = 0; i < WORKER_COUNT; i++) {
        if (workers[i].running) {
            if (kill(workers[i].pid, SIGTERM) == -1 &&
                errno != ESRCH) {
                perror("kill SIGTERM");
            }
        }
    }

    time_t deadline = time(NULL) + SHUTDOWN_TIMEOUT;

    /* Wait up to five seconds for workers to exit */
    while (any_worker_running() &&
           time(NULL) < deadline) {

        check_children();

        if (!any_worker_running())
            break;

        struct timespec delay = {0, 100000000L};
        nanosleep(&delay, NULL);
    }

    /* Force termination if workers are still alive */
    if (any_worker_running()) {
        printf("Timeout: sending SIGKILL.\n");

        for (int i = 0; i < WORKER_COUNT; i++) {
            if (workers[i].running) {
                if (kill(workers[i].pid, SIGKILL) == -1 &&
                    errno != ESRCH) {
                    perror("kill SIGKILL");
                }
            }
        }

        /* Reap remaining children */
        for (int i = 0; i < WORKER_COUNT; i++) {
            if (!workers[i].running)
                continue;

            int status;
            pid_t result;

            do {
                result = waitpid(workers[i].pid,
                                 &status, 0);
            } while (result == -1 && errno == EINTR);

            if (result == workers[i].pid) {
                workers[i].running = 0;
                workers[i].pid = 0;
            }
            else if (result == -1) {
                perror("waitpid shutdown");
            }
        }
    }

    printf("Watchdog shutdown complete.\n");
}

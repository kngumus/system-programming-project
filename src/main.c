
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include "signal_manager.h"
#include "process_manager.h"
#include "logger.h"

int main(void)
{
    /* Initialize logging */
    if (logger_init("watchdog.log") == -1) {
        fprintf(stderr, "Logger initialization failed.\n");
        return EXIT_FAILURE;
    }

    logger_log(LOG_INFO, "Watchdog started (PID: %ld)",
               (long)getpid());

    printf("Watchdog started. Parent PID: %ld\n",
           (long)getpid());

    /* Install signal handlers */
    if (setup_signal_handlers() == -1) {
        perror("sigaction");
        logger_log(LOG_ERROR, "Failed to install signal handlers");
        logger_close();
        return EXIT_FAILURE;
    }

    /* Configure a periodic timer */
    struct itimerval timer = {0};

    timer.it_value.tv_sec = 1;
    timer.it_interval.tv_sec = 1;

    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        perror("setitimer");
        logger_log(LOG_ERROR, "Failed to configure periodic timer");
        logger_close();
        return EXIT_FAILURE;
    }

    /* Create the initial workers */
    if (start_all_workers() == -1) {
        logger_log(LOG_ERROR, "Failed to start all workers");
        shutdown_requested = 1;

        /* Stop timer if worker startup fails */
        struct itimerval stop_timer = {0};

        if (setitimer(ITIMER_REAL, &stop_timer, NULL) == -1) {
            perror("setitimer stop");
            logger_log(LOG_ERROR, "Failed to stop periodic timer");
        }

        shutdown_workers();
        logger_close();
        return EXIT_FAILURE;
    }

    /* Main monitoring loop */
    while (!shutdown_requested) {

        /* Handle terminated child processes */
        if (sigchld_received) {
            sigchld_received = 0;
            check_children();
        }

        /* Handle status report requests */
        if (sigusr1_received) {
            sigusr1_received = 0;
            check_children();

            if (!shutdown_requested)
                print_worker_status();
        }

        /* Periodic monitoring triggered by SIGALRM */
        if (sigalrm_received) {
            sigalrm_received = 0;

            if (!shutdown_requested)
                check_children();
        }

        /* Sleep briefly to avoid busy waiting */
        struct timespec delay = {0, 100000000L};
        nanosleep(&delay, NULL);
    }

    logger_log(LOG_INFO, "Watchdog shutdown initiated");

    /* Stop periodic timer */
    struct itimerval stop_timer = {0};

    if (setitimer(ITIMER_REAL, &stop_timer, NULL) == -1) {
        perror("setitimer stop");
        logger_log(LOG_ERROR, "Failed to stop periodic timer");
    }

    /* Stop workers without restarting them */
    shutdown_workers();

    logger_log(LOG_INFO, "Watchdog shutdown completed");
    logger_close();

    return EXIT_SUCCESS;
}

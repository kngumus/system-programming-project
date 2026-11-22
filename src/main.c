
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "signal_manager.h"
#include "process_manager.h"
#include "logger.h"

int main(void)
{

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


    /* Create the initial workers */
    
    if (start_all_workers() == -1) {
        logger_log(LOG_ERROR, "Failed to start all workers");
        shutdown_requested = 1;
        shutdown_workers();
        logger_close();
        return EXIT_FAILURE;
    }


    /* Main monitoring loop */
    while (!shutdown_requested) {

        if (sigchld_received) {
            sigchld_received = 0;
            check_children();
        }

        if (sigusr1_received) {
            sigusr1_received = 0;
            check_children();

            if (!shutdown_requested)
                print_worker_status();
        }

        /* Check for terminated children */
        if (!shutdown_requested)
            check_children();

        /* Sleep briefly to avoid busy waiting */
        struct timespec delay = {0, 100000000L};
        nanosleep(&delay, NULL);
    }

    /* Stop workers without restarting them */
    
    logger_log(LOG_INFO, "Watchdog shutdown initiated");

    shutdown_workers();

    logger_log(LOG_INFO, "Watchdog shutdown completed");
    logger_close();

    return EXIT_SUCCESS;
    ;
}

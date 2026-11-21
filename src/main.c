
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "signal_manager.h"
#include "process_manager.h"

int main(void)
{
    printf("Watchdog started. Parent PID: %ld\n",
           (long)getpid());

    /* Install signal handlers */
    if (setup_signal_handlers() == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    /* Create the initial workers */
    if (start_all_workers() == -1) {
        shutdown_requested = 1;
        shutdown_workers();
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
    shutdown_workers();

    return EXIT_SUCCESS;
}

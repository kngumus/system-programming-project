
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
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

    /* Configure a periodic timer */
    struct itimerval timer = {0};

    timer.it_value.tv_sec = 1;
    timer.it_interval.tv_sec = 1;

    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        perror("setitimer");
        return EXIT_FAILURE;
    }

    
    /* Create the initial workers */
    if (start_all_workers() == -1) {
        shutdown_requested = 1;

        /* Stop the timer if worker startup fails */
        struct itimerval stop_timer = {0};

        if (setitimer(ITIMER_REAL, &stop_timer, NULL) == -1) {
            perror("setitimer stop");
        }

        shutdown_workers();
        return EXIT_FAILURE;
    }



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


    
    /* Stop the periodic timer */
    struct itimerval stop_timer = {0};

    if (setitimer(ITIMER_REAL, &stop_timer, NULL) == -1) {
        perror("setitimer stop");
    }

    /* Stop workers without restarting them */
    shutdown_workers();


    return EXIT_SUCCESS;
}

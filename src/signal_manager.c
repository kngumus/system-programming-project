
#include <stddef.h>
#include "signal_manager.h"

/* Flags set by signal handlers */
volatile sig_atomic_t sigchld_received = 0;
volatile sig_atomic_t sigusr1_received = 0;
volatile sig_atomic_t shutdown_requested = 0;
volatile sig_atomic_t sigalrm_received = 0;

/* Signal handler */
static void signal_handler(int signo)
{
    if (signo == SIGCHLD) {
        sigchld_received = 1;
    }
    else if (signo == SIGUSR1) {
        sigusr1_received = 1;
    }
    else if (signo == SIGINT || signo == SIGTERM) {
        shutdown_requested = 1;
    }
    else if (signo == SIGALRM) {
        sigalrm_received = 1;
    }
}

/* Install signal handlers */
int setup_signal_handlers(void)
{
    struct sigaction sa = {0};

    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGCHLD, &sa, NULL) == -1)
        return -1;

    if (sigaction(SIGUSR1, &sa, NULL) == -1)
        return -1;

    if (sigaction(SIGINT, &sa, NULL) == -1)
        return -1;

    if (sigaction(SIGTERM, &sa, NULL) == -1)
        return -1;

    if (sigaction(SIGALRM, &sa, NULL) == -1)
        return -1;

    return 0;
}

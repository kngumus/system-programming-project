#ifndef SIGNAL_MANAGER_H
#define SIGNAL_MANAGER_H

#include <signal.h>

/* Flags set by signal handlers */
extern volatile sig_atomic_t sigchld_received;
extern volatile sig_atomic_t sigusr1_received;
extern volatile sig_atomic_t shutdown_requested;
extern volatile sig_atomic_t sigalrm_received;

/* Register POSIX signal handlers */
int setup_signal_handlers(void);

#endif

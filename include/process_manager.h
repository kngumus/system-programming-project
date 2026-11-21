
#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include <sys/types.h>

#define WORKER_COUNT 2
#define MAX_RESTARTS 3

typedef struct {
    pid_t pid;
    int restart_count;
    int running;
} Worker;

int start_all_workers(void);
void check_children(void);
void print_worker_status(void);
void shutdown_workers(void);

#endif

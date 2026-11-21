
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Missing worker ID\n");
        return EXIT_FAILURE;
    }

    printf("Worker %s running (PID: %ld)\n",
           argv[1], (long)getpid());

    fflush(stdout);

    while (1) {
        pause();
    }

    return EXIT_SUCCESS;
}

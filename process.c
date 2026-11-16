/*
 * =====================================================================================
 *
 *       Filename:  process.c
 *
 *    Description:  Standard workload process simulator for Operating Systems dispatcher.
 *                  Supports SIGINT (termination), SIGTSTP (suspend), and SIGCONT (resume).
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

void signal_handler(int sig) {
    if (sig == SIGINT) {
        printf("   [process PID %d] Received SIGINT (termination). Exiting.\n", getpid());
        fflush(stdout);
        exit(0);
    } else if (sig == SIGTSTP) {
        printf("   [process PID %d] Received SIGTSTP (suspended).\n", getpid());
        fflush(stdout);
        pause(); // Wait for SIGCONT
    } else if (sig == SIGCONT) {
        printf("   [process PID %d] Received SIGCONT (resumed).\n", getpid());
        fflush(stdout);
    }
}

int main(int argc, char *argv[]) {
    // Configure signal handling
    signal(SIGINT, signal_handler);
    signal(SIGTSTP, signal_handler);
    signal(SIGCONT, signal_handler);

    int cputime = 5;
    if (argc > 1) {
        cputime = atoi(argv[1]);
        if (cputime <= 0) cputime = 1;
    }

    printf("   [process PID %d] Running simulated CPU burst (%d seconds)...\n", getpid(), cputime);
    fflush(stdout);

    while (cputime > 0) {
        sleep(1);
        cputime--;
    }

    printf("   [process PID %d] CPU burst complete.\n", getpid());
    fflush(stdout);
    return 0;
}

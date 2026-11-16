/*
 * =====================================================================================
 *
 *       Filename:  sleepy.c
 *
 *    Description:  Simulated workload process for batch process dispatcher.
 *                  Sleeps for the requested number of seconds and handles SIGINT.
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

/* Signal handler for SIGINT */
void handle_sigint(int sig) {
    printf("   [sleepy PID %d] Caught SIGINT (time expired). Cleaning up and exiting.\n", getpid());
    fflush(stdout);
    exit(0);
}

int main(int argc, char *argv[]) {
    // Install SIGINT handler
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    int duration = 10;
    if (argc > 1) {
        duration = atoi(argv[1]);
        if (duration <= 0) duration = 1;
    }

    printf("   [sleepy PID %d] Started execution with requested burst = %d seconds.\n", getpid(), duration);
    fflush(stdout);

    for (int i = 0; i < duration; i++) {
        sleep(1);
    }

    printf("   [sleepy PID %d] Completed burst naturally.\n", getpid());
    fflush(stdout);
    return 0;
}

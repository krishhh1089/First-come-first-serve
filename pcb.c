#include "pcb.h"
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

/*
 * Allocate and initialize a null PCB structure
 */
PcbPtr createnullPcb(void) {
    PcbPtr p = (PcbPtr) malloc(sizeof(Pcb));
    if (!p) {
        perror("malloc failed for PCB");
        exit(EXIT_FAILURE);
    }
    p->pid = 0;
    for (int i = 0; i < MAXARGS; i++) {
        p->args[i] = NULL;
    }
    p->arrivaltime = 0;
    p->remainingcputime = 0;
    p->next = NULL;
    p->priority = 0;
    p->memalloc = 64;
    p->res1 = 0;
    p->res2 = 0;
    p->res3 = 0;
    p->res4 = 0;
    return p;
}

/*
 * Free a PCB structure and any allocated argument strings
 */
void freePcb(PcbPtr p) {
    if (!p) return;
    for (int i = 0; i < MAXARGS; i++) {
        if (p->args[i]) {
            free(p->args[i]);
            p->args[i] = NULL;
        }
    }
    free(p);
}

/*
 * Start a process by forking a child and executing the program
 */
PcbPtr startPcb(PcbPtr p) {
    if (!p || !p->args[0]) return NULL;

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork() failed");
        return NULL;
    }

    if (pid == 0) {
        // In Child Process
        // Try direct execution first via PATH or relative path
        execvp(p->args[0], p->args);

        // If execvp failed and argument didn't contain path separator, retry with "./"
        if (p->args[0][0] != '.' && p->args[0][0] != '/') {
            char local_path[512];
            snprintf(local_path, sizeof(local_path), "./%s", p->args[0]);
            execv(local_path, p->args);
        }

        fprintf(stderr, "Error: Failed to exec '%s'\n", p->args[0]);
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    // In Parent Process
    p->pid = pid;
    return p;
}

/*
 * Terminate a process by sending SIGINT, awaiting its termination, and freeing memory
 */
PcbPtr terminatePcb(PcbPtr p) {
    if (!p) return NULL;

    if (p->pid > 0) {
        // Send SIGINT to the process as mandated by FCFS algorithm step 4.i.b.A
        kill(p->pid, SIGINT);
        int status = 0;
        waitpid(p->pid, &status, WUNTRACED);
    }

    freePcb(p);
    return NULL;
}

/*
 * Diagnostic print of PCB contents
 */
void printPcb(const PcbPtr p) {
    if (!p) {
        printf("[PCB: NULL]\n");
        return;
    }
    printf("[PCB PID=%d, Arrival=%d, RemainingCPU=%d, Priority=%d, Memory=%dMB, Cmd='",
           p->pid, p->arrivaltime, p->remainingcputime, p->priority, p->memalloc);
    for (int i = 0; p->args[i] != NULL && i < MAXARGS; i++) {
        printf("%s%s", (i > 0 ? " " : ""), p->args[i]);
    }
    printf("']\n");
}

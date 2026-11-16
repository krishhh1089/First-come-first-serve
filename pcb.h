#ifndef PCB_H
#define PCB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define MAXARGS 64

/*
 * Process Control Block (PCB) structure
 * Defines the attributes required by the First-Come-First-Served (FCFS) dispatcher
 * and accommodates multi-resource parameters for subsequent stages.
 */
struct pcb {
    pid_t pid;             // System process ID
    char * args[MAXARGS];  // Program name and arguments (NULL-terminated)
    int arrivaltime;       // Time when the process arrives into the dispatcher
    int remainingcputime;  // CPU burst time remaining to execute
    struct pcb * next;     // Links for PCB queue / linked-list handlers

    // Additional process attributes (for full host dispatcher format)
    int priority;          // Process priority level
    int memalloc;          // Memory allocation requirement (MB)
    int res1;              // Device/resource 1 allocation
    int res2;              // Device/resource 2 allocation
    int res3;              // Device/resource 3 allocation
    int res4;              // Device/resource 4 allocation
};

typedef struct pcb Pcb;
typedef Pcb * PcbPtr;

/* PCB Helper Functions */
PcbPtr createnullPcb(void);
PcbPtr startPcb(PcbPtr p);
PcbPtr terminatePcb(PcbPtr p);
void freePcb(PcbPtr p);
void printPcb(const PcbPtr p);

#endif /* PCB_H */

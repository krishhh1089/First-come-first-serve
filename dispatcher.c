/*
 * =====================================================================================
 *
 *       Filename:  dispatcher.c
 *
 *    Description:  First-Come-First-Served (FCFS) Batch Process Dispatcher
 *                  Implements the exact 5-step scheduling algorithm from the
 *                  Operating Systems coursework specifications.
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "pcb.h"
#include "queue.h"

#define BUFFER_SIZE 512

/* Trim whitespace from both ends of string */
static char *trim_whitespace(char *str) {
    if (!str) return NULL;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

/*
 * Step 2: Fill dispatcher queue from dispatch list file
 * Automatically handles:
 * Format 1: <arrival>,<program name> <args...>   (e.g., "0,sleepy 3")
 * Format 2: <arrival>,<priority>,<cputime>,<memory alloc>,<res1>,<res2>,<res3>,<res4>
 */
int fillDispatcherQueue(Queue *q, const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Error opening dispatch list file");
        return -1;
    }

    char line[BUFFER_SIZE];
    int job_count = 0;

    printf("\n======================================================\n");
    printf("   LOADING DISPATCH LIST FILE: %s\n", filename);
    printf("======================================================\n");

    while (fgets(line, sizeof(line), fp)) {
        char *trimmed = trim_whitespace(line);
        if (strlen(trimmed) == 0 || trimmed[0] == '#') {
            continue; // Skip comments and empty lines
        }

        // Locate first comma delimiter
        char *comma_ptr = strchr(trimmed, ',');
        if (!comma_ptr) {
            fprintf(stderr, "Warning: Skipping malformed line (no comma found): %s\n", trimmed);
            continue;
        }

        *comma_ptr = '\0';
        char *arrival_str = trim_whitespace(trimmed);
        char *rest = trim_whitespace(comma_ptr + 1);

        PcbPtr p = createnullPcb();
        p->arrivaltime = atoi(arrival_str);

        // Check if the remainder is 7 comma-separated values (Project 2 full format)
        int comma_count = 0;
        for (int i = 0; rest[i]; i++) {
            if (rest[i] == ',') comma_count++;
        }

        if (comma_count >= 6) {
            // Format 2: <priority>,<cputime>,<mem>,<r1>,<r2>,<r3>,<r4>
            int priority = 0, cputime = 0, mem = 64, r1 = 0, r2 = 0, r3 = 0, r4 = 0;
            int matched = sscanf(rest, "%d , %d , %d , %d , %d , %d , %d",
                                 &priority, &cputime, &mem, &r1, &r2, &r3, &r4);
            if (matched < 2) {
                fprintf(stderr, "Warning: Failed to parse full format line: %s\n", rest);
                freePcb(p);
                continue;
            }
            p->priority = priority;
            p->remainingcputime = cputime;
            p->memalloc = mem;
            p->res1 = r1;
            p->res2 = r2;
            p->res3 = r3;
            p->res4 = r4;

            // Default program command for full format
            p->args[0] = strdup("./process");
            char burst_str[32];
            snprintf(burst_str, sizeof(burst_str), "%d", cputime);
            p->args[1] = strdup(burst_str);
            p->args[2] = NULL;
        } else {
            // Format 1: <command> [args...] (e.g. "sleepy 3" or "./sleepy 3")
            int arg_idx = 0;
            char *token = strtok(rest, " \t");
            while (token && arg_idx < MAXARGS - 1) {
                p->args[arg_idx++] = strdup(token);
                token = strtok(NULL, " \t");
            }
            p->args[arg_idx] = NULL;

            // Determine burst time: if second argument is numeric (e.g., "sleepy 3"), use it
            if (arg_idx >= 2) {
                p->remainingcputime = atoi(p->args[1]);
            }
            if (p->remainingcputime <= 0) {
                p->remainingcputime = 1; // Default minimum 1 second
            }
        }

        enqueue(q, p);
        job_count++;
        printf(" [Job %2d Loaded] Arrival: %2d s | Burst: %2d s | Command: %s %s\n",
               job_count, p->arrivaltime, p->remainingcputime,
               p->args[0] ? p->args[0] : "",
               p->args[1] ? p->args[1] : "");
    }

    fclose(fp);
    printf("Total jobs queued: %d\n", job_count);
    printf("======================================================\n\n");
    return job_count;
}

/*
 * Main Dispatcher execution routine
 */
int main(int argc, char *argv[]) {
    const char *dispatch_file = "dispatchlist.txt";

    if (argc >= 2) {
        dispatch_file = argv[1];
    } else {
        printf("Usage: %s <dispatch_list_file>\n", argv[0]);
        printf("Defaulting to input file: '%s'\n\n", dispatch_file);
    }

    // Step 1: Initialize dispatcher queue
    Queue dispatcher_queue;
    initQueue(&dispatcher_queue);

    // Step 2: Fill dispatcher queue from dispatch list file
    int total_jobs = fillDispatcherQueue(&dispatcher_queue, dispatch_file);
    if (total_jobs <= 0) {
        fprintf(stderr, "Error: Dispatcher queue empty or invalid input file. Exiting.\n");
        return EXIT_FAILURE;
    }

    // Step 3: Start dispatcher timer
    int dispatcher_timer = 0;
    PcbPtr current_process = NULL;

    printf("Starting FCFS Dispatcher execution loop...\n");
    printf("--------------------------------------------------------------------------------\n");
    printf(" Time | Event Description\n");
    printf("--------------------------------------------------------------------------------\n");

    // Step 4: While there's anything in the queue or there is a currently running process:
    while (!isEmpty(&dispatcher_queue) || current_process != NULL) {

        // Step 4.i: If a process is currently running
        if (current_process != NULL) {
            // Step 4.i.a: Decrement process remainingcputime
            current_process->remainingcputime--;

            // Step 4.i.b: If times up:
            if (current_process->remainingcputime <= 0) {
                printf("[%3d s] Process PID %5d ('%s') completed its CPU burst.\n",
                       dispatcher_timer, current_process->pid, current_process->args[0]);
                printf("[%3d s] --> Sending SIGINT to PID %d and freeing PCB memory.\n",
                       dispatcher_timer, current_process->pid);

                // Step 4.i.b.A & B: Send SIGINT and free PCB memory
                terminatePcb(current_process);
                current_process = NULL;
            } else {
                printf("[%3d s] Process PID %5d ('%s') active | Remaining CPU: %d s\n",
                       dispatcher_timer, current_process->pid, current_process->args[0],
                       current_process->remainingcputime);
            }
        }

        // Step 4.ii: If no process currently running && dispatcher queue is not empty
        //            && arrivaltime of process at head of queue <= dispatcher timer:
        if (current_process == NULL && !isEmpty(&dispatcher_queue)) {
            PcbPtr head_process = peek(&dispatcher_queue);
            if (head_process->arrivaltime <= dispatcher_timer) {
                // Step 4.ii.a: Dequeue process and start it (fork & exec)
                current_process = dequeue(&dispatcher_queue);
                printf("[%3d s] Dequeued job from ready queue (Arrival=%d s, Burst=%d s).\n",
                       dispatcher_timer, current_process->arrivaltime, current_process->remainingcputime);

                startPcb(current_process);

                // Step 4.ii.b: Set it as currently running process
                printf("[%3d s] --> Started child process PID %5d ('%s' %s)\n",
                       dispatcher_timer, current_process->pid,
                       current_process->args[0],
                       current_process->args[1] ? current_process->args[1] : "");
            } else {
                printf("[%3d s] CPU Idle (Next job arrives at time %d s).\n",
                       dispatcher_timer, head_process->arrivaltime);
            }
        } else if (current_process == NULL && isEmpty(&dispatcher_queue)) {
            // Queue is empty and no process is running
            break;
        }

        // Step 4.iii: sleep for one second
        sleep(1);

        // Step 4.iv: Increment dispatcher timer
        dispatcher_timer++;

        // Step 4.v: Go back to 4
    }

    printf("--------------------------------------------------------------------------------\n");
    // Step 5: Exit
    printf("[%3d s] All jobs have been dispatched and completed. Dispatcher exiting successfully.\n",
           dispatcher_timer);
    printf("================================================================================\n\n");

    return EXIT_SUCCESS;
}

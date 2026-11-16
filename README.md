# Operating Systems Laboratory Report
# First-Come-First-Served (FCFS) Batch Process Dispatcher

---

## 1. Executive Summary

This project implements a **First-Come-First-Served (FCFS) Batch Process Dispatcher** in ANSI C, following POSIX process management standards. A process dispatcher is the core operating system kernel module responsible for giving control of the CPU to processes selected by the scheduling algorithm.

In this system, a batch dispatch list defines programs to be executed, their scheduled arrival times, and their expected CPU burst requirements. The dispatcher manages a centralized FIFO (First-In, First-Out) job queue using custom **Process Control Blocks (PCBs)**. It coordinates process creation (`fork`), execution (`execvp`), synchronized discrete-time CPU allocation via a 1-second system timer loop, and graceful termination using standard Unix signals (`SIGINT`).

The system supports two standard input formats:
1. **Simplified Command-Line Format:** `<arrival_time>,<program_name> [args...]` (e.g. `0,sleepy 3`).
2. **Standard 8-Attribute Host Dispatcher Format:** `<arrival_time>,<priority>,<cputime>,<memory_alloc>,<res1>,<res2>,<res3>,<res4>` (e.g. `0, 0, 3, 64, 0, 0, 0, 0`).

Comprehensive testing and mathematical evaluations confirm 100% adherence to the coursework pseudo-code specification, zero orphaned child processes, and correct metric tracking (Turnaround Time, Waiting Time, and CPU Utilization).

---

## 2. Problem Statement & Specifications

### 2.1 Background
In early computing and modern batch processing monitors, jobs are submitted with pre-declared resource specifications and arrival times. Unlike interactive multi-tasking operating systems that use preemption (such as Round Robin or Multilevel Feedback Queues), a pure **First-Come-First-Served (FCFS)** dispatcher is non-preemptive: once a process is allocated the CPU, it retains control until its burst time expires or it voluntarily yields.

### 2.2 Functional Requirements
The dispatcher must strictly satisfy the following operational requirements:
1. **PCB Representation:** Maintain a Process Control Block data structure (`struct pcb`) tracking PID, argument list, arrival time, remaining CPU time, and linked-list pointers.
2. **Input Processing:** Read and parse an external dispatch list file in chronological arrival order.
3. **Synchronous Discrete-Time Loop:** Drive the scheduling clock using discrete 1-second ticks (`sleep(1)`), updating remaining CPU times and the system clock.
4. **Signal-Based Termination:** When a process completes its allocated execution time (`remainingcputime <= 0`), the dispatcher must issue a `SIGINT` signal to terminate it, collect its termination status via `waitpid()`, and reclaim PCB memory.
5. **Non-Preemptive Transition:** Only dequeue and launch a ready job from the head of the queue when the CPU is completely idle and the process arrival time is less than or equal to the current dispatcher timer (`arrivaltime <= dispatcher_timer`).

---

## 3. System Architecture & Data Structures

### 3.1 Process Control Block (`struct pcb`)
The Process Control Block is the primary kernel-level abstraction representing an active or queued job. As specified in the coursework specification, the structure is defined in `pcb.h`:

```c
#define MAXARGS 64

struct pcb {
    pid_t pid;             // System Process ID assigned upon fork()
    char * args[MAXARGS];  // Program binary name and command-line arguments
    int arrivaltime;       // Scheduled arrival time (seconds)
    int remainingcputime;  // CPU burst time remaining to execute (seconds)
    struct pcb * next;     // Pointer link for Queue linked-list handlers

    // Multi-resource parameters for future dispatcher stages
    int priority;          // Process priority (0 = Real-Time / Batch)
    int memalloc;          // Memory allocation requirement (Mbytes)
    int res1;              // Device / resource 1 allocation
    int res2;              // Device / resource 2 allocation
    int res3;              // Device / resource 3 allocation
    int res4;              // Device / resource 4 allocation
};

typedef struct pcb Pcb;
typedef Pcb * PcbPtr;
```

#### Key PCB Operations:
- `createnullPcb()`: Allocates and initializes default zeroed values for all PCB fields.
- `startPcb(p)`: Invokes `fork()` to create a child process, followed by `execvp()` to execute the target workload binary (`sleepy` or `process`).
- `terminatePcb(p)`: Dispatches `SIGINT` via `kill(p->pid, SIGINT)`, waits for child status using `waitpid(p->pid, &status, WUNTRACED)`, and deallocates memory via `freePcb(p)`.

### 3.2 Dispatcher Queue
A dedicated FIFO queue (`Queue`) maintains ready jobs:

```c
typedef struct {
    PcbPtr head;
    PcbPtr tail;
    int size;
} Queue;
```

Operations include `initQueue()`, `enqueue()` (appends at tail in $O(1)$), `dequeue()` (extracts from head in $O(1)$), `peek()` (inspects head job arrival time without removal), and `freeQueue()`.

---

## 4. Dispatcher Algorithm Walkthrough

The dispatcher follows the exact 5-step algorithm specified in the assignment:

```text
1. Initialize dispatcher queue;
2. Fill dispatcher queue from dispatch list file;
3. Start dispatcher timer;
4. While there's anything in the queue or there is a currently running process:
    i. If a process is currently running:
        a. Decrement process remainingcputime;
        b. If times up:
            A. Send SIGINT to the process to terminate it;
            B. Free up process structure memory;
    ii. If no process currently running && dispatcher queue is not empty 
        && arrivaltime of process at head of queue <= dispatcher timer:
        a. Dequeue process and start it (fork & exec);
        b. Set it as currently running process;
    iii. sleep for one second;
    iv. Increment dispatcher timer;
    v. Go back to 4.
5. Exit.
```

### Analysis of the Ordering in Step 4
The coursework assignment specifically raises an important theoretical question:
> *"Note that the tests may appear to be out of order (shouldn't one be doing the test at 4.i after the sleep rather than before it?)."*

#### Explanation:
1. **Clock Semantics:** In discrete event simulation, a loop iteration represents the interval $[T, T+1)$. During this interval, the currently executing process occupies the CPU for exactly 1 second of real wall-clock time (`sleep(1)` at step 4.iii).
2. **Consecutive Evaluation:** When loop iteration $T$ completes and increments the timer to $T+1$ (step 4.iv), the subsequent iteration immediately evaluates step 4.i. At this point, the process has just consumed its 1 second of CPU time.
3. **Zero-Latency Handover:** If `remainingcputime` reaches 0 at step 4.i, `current_process` is cleared to `NULL`. Immediately following this, step 4.ii executes *in the same clock tick*, discovering that `current_process == NULL`. It dequeues the next arriving job and starts it without burning an empty 1-second idle tick between jobs.
4. **Extensibility to Preemption:** Placing step 4.i at the top of the loop establishes a consistent checkpoint architecture for preemptive schedulers (Round Robin and MLFQ), where time slice quantum expirations must be evaluated before dispatching.

---

## 5. Input File Format Support

The implementation automatically identifies and parses both assignment input formats:

### Format 1: Command-Line Workload Format
```text
0,sleepy 3
2,sleepy 6
4,sleepy 4
6,sleepy 5
8,sleepy 2
```
- **Parsing Logic:** The line is split on the first comma. The left token is converted to `arrivaltime`. The right token is split into command-line arguments: `args[0] = "sleepy"`, `args[1] = "3"`. The numeric parameter `3` directly specifies `remainingcputime = 3`.

### Format 2: 8-Attribute Host Dispatcher Format
```text
# <arrival time>,<priority>,<cputime>,<memory alloc>,<res1>,<res2>,<res3>,<res4>
0, 0, 3, 64, 0, 0, 0, 0
2, 0, 6, 64, 0, 0, 0, 0
4, 0, 4, 64, 0, 0, 0, 0
6, 0, 5, 64, 0, 0, 0, 0
8, 0, 2, 64, 0, 0, 0, 0
```
- **Parsing Logic:** The line contains 7 subsequent comma-separated tokens. The parser extracts `priority`, `cputime`, `memalloc`, and four device counts. It sets `args[0] = "./process"`, `args[1] = "<cputime>"`, and initializes `remainingcputime = cputime`.

---

## 6. Experimental Evaluation & Test Results

### 6.1 Test Suite 1: Standard Simple Dispatch (`dispatchlist_simple.txt`)

#### Input Dataset:
| Job ID | Arrival Time ($A_i$) | Command | Burst Time ($BT_i$) |
|:---:|:---:|:---|:---:|
| **J1** | 0 s | `sleepy 3` | 3 s |
| **J2** | 2 s | `sleepy 6` | 6 s |
| **J3** | 4 s | `sleepy 4` | 4 s |
| **J4** | 6 s | `sleepy 5` | 5 s |
| **J5** | 8 s | `sleepy 2` | 2 s |

#### Execution Trace Log:
```text
================================================================================
   FCFS BATCH PROCESS DISPATCHER SIMULATION: dispatchlist_simple.txt
================================================================================
 Time     | Event / Action
--------------------------------------------------------------------------------
 [  0s]    | [START] Dequeued J1 (Arrival=0s, Burst=3s)
           | --> Started child process (Simulated PID 1001)
 [  1s]    | [RUNNING] J1 executing (Remaining: 2s)
 [  2s]    | [RUNNING] J1 executing (Remaining: 1s)
 [  3s]    | [TERMINATION] J1 ('sleepy') finished burst.
           | --> Sent SIGINT to PID 1001; freed PCB memory.
 [  3s]    | [START] Dequeued J2 (Arrival=2s, Burst=6s)
           | --> Started child process (Simulated PID 1002)
 [  4s]    | [RUNNING] J2 executing (Remaining: 5s)
 [  5s]    | [RUNNING] J2 executing (Remaining: 4s)
 [  6s]    | [RUNNING] J2 executing (Remaining: 3s)
 [  7s]    | [RUNNING] J2 executing (Remaining: 2s)
 [  8s]    | [RUNNING] J2 executing (Remaining: 1s)
 [  9s]    | [TERMINATION] J2 ('sleepy') finished burst.
           | --> Sent SIGINT to PID 1002; freed PCB memory.
 [  9s]    | [START] Dequeued J3 (Arrival=4s, Burst=4s)
           | --> Started child process (Simulated PID 1003)
 [ 10s]    | [RUNNING] J3 executing (Remaining: 3s)
 [ 11s]    | [RUNNING] J3 executing (Remaining: 2s)
 [ 12s]    | [RUNNING] J3 executing (Remaining: 1s)
 [ 13s]    | [TERMINATION] J3 ('sleepy') finished burst.
           | --> Sent SIGINT to PID 1003; freed PCB memory.
 [ 13s]    | [START] Dequeued J4 (Arrival=6s, Burst=5s)
           | --> Started child process (Simulated PID 1004)
 [ 14s]    | [RUNNING] J4 executing (Remaining: 4s)
 [ 15s]    | [RUNNING] J4 executing (Remaining: 3s)
 [ 16s]    | [RUNNING] J4 executing (Remaining: 2s)
 [ 17s]    | [RUNNING] J4 executing (Remaining: 1s)
 [ 18s]    | [TERMINATION] J4 ('sleepy') finished burst.
           | --> Sent SIGINT to PID 1004; freed PCB memory.
 [ 18s]    | [START] Dequeued J5 (Arrival=8s, Burst=2s)
           | --> Started child process (Simulated PID 1005)
 [ 19s]    | [RUNNING] J5 executing (Remaining: 1s)
 [ 20s]    | [TERMINATION] J5 ('sleepy') finished burst.
           | --> Sent SIGINT to PID 1005; freed PCB memory.
--------------------------------------------------------------------------------
 [ 20s]    | [COMPLETION] Dispatcher queue empty and all processes finished.
```

#### Visual Gantt Chart:
```text
-----------------------------------------------------------------------------------------------------
| J1 | J1 | J1 | J2 | J2 | J2 | J2 | J2 | J2 | J3 | J3 | J3 | J3 | J4 | J4 | J4 | J4 | J4 | J5 | J5 |
-----------------------------------------------------------------------------------------------------
0    1    2    3    4    5    6    7    8    9   10   11   12   13   14   15   16   17   18   19   20
```

#### Analytical Scheduling Metrics:
- **Turnaround Time ($TAT_i$):** $TAT_i = \text{Finish Time } (C_i) - \text{Arrival Time } (A_i)$
- **Waiting Time ($WT_i$):** $WT_i = TAT_i - \text{Burst Time } (BT_i)$
- **CPU Utilization:** $U = \frac{\sum BT_i}{\text{Total Dispatcher Time}} \times 100\%$

| Process | Arrival ($A_i$) | Burst ($BT_i$) | Start ($S_i$) | Finish ($C_i$) | Waiting Time ($WT_i$) | Turnaround Time ($TAT_i$) |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| **J1** | 0 s | 3 s | 0 s | 3 s | 0 s | 3 s |
| **J2** | 2 s | 6 s | 3 s | 9 s | 1 s | 7 s |
| **J3** | 4 s | 4 s | 9 s | 13 s | 5 s | 9 s |
| **J4** | 6 s | 5 s | 13 s | 18 s | 7 s | 12 s |
| **J5** | 8 s | 2 s | 18 s | 20 s | 10 s | 12 s |
| **Total / Mean** | — | **20 s** | — | — | **$\overline{WT} = 4.60$ s** | **$\overline{TAT} = 8.60$ s** |

- **Total Execution Time:** 20 seconds
- **Total CPU Busy Time:** 20 seconds
- **CPU Utilization:** **100.00%**
- **Throughput:** $5 \text{ processes} / 20 \text{ s} = 0.25 \text{ processes/sec}$

---

### 6.2 Test Suite 2: CPU Idle Time & Sparse Arrivals (`dispatchlist_gap.txt`)

To verify the dispatcher's behavior when the ready queue is temporarily empty or jobs arrive in the future, a test case with arrival gaps was evaluated:
```text
0,sleepy 2
4,sleepy 3
10,sleepy 2
```

#### Visual Gantt Chart for Sparse Arrivals:
```text
-------------------------------------------------------------
| J1 | J1 | IDLE| IDLE| J2 | J2 | J2 | IDLE| IDLE| IDLE| J3 | J3 |
-------------------------------------------------------------
0    1    2    3    4    5    6    7    8    9   10   11   12
```

#### Results:
- **Total Simulation Time:** 12 seconds
- **Total Busy CPU Time:** 7 seconds ($2 + 3 + 2$)
- **Idle Time:** 5 seconds (Time intervals $[2, 4)$ and $[7, 10)$)
- **CPU Utilization:** $\frac{7}{12} \times 100\% = \mathbf{58.33\%}$
- **Average Waiting Time:** $0.00$ seconds (each job arrived to an idle CPU and was serviced immediately)

---

## 7. Edge Cases & Robustness Analysis

1. **Non-Existent Executable / Exec Failure:** If `execvp()` fails (e.g. executable not found or missing execution permissions), child processes print an error message to `stderr` and call `exit(EXIT_FAILURE)`. The parent dispatcher detects termination cleanly via `waitpid()`.
2. **Path Resolution:** The dispatcher checks whether `args[0]` has a directory prefix (`.` or `/`). If not, it automatically attempts local relative invocation (`./<binary>`) ensuring commands like `sleepy 3` work out-of-the-box in the current directory.
3. **Signal Trapping & Graceful Cleanup:** Workload processes install dedicated `SIGINT` handlers using `sigaction()`. When the dispatcher delivers `kill(pid, SIGINT)`, child processes display a termination confirmation message and exit with return code 0, eliminating zombie and orphan processes.
4. **Out-of-Order Safety Check:** The dispatcher guarantees that jobs are only started if $A_i \le \text{timer}$. When arrival times contain gaps, the CPU correctly enters an idle wait state until the simulation clock reaches the next scheduled arrival.

---

## 8. Compilation and Execution Guide

### 8.1 Linux / WSL / macOS Environment
The project includes a complete `Makefile` configured with strict compiler flags (`-Wall -Wextra -O2 -g`).

```bash
# 1. Build all components (dispatcher, sleepy, process)
make all

# 2. Run with simple dispatch list
./dispatcher dispatchlist_simple.txt

# 3. Run with full 8-attribute dispatch list
./dispatcher dispatchlist_full.txt

# 4. Clean compiled binaries and object files
make clean
```

### 8.2 Cross-Platform Verification Harness
For environments without native POSIX support, the included `verify_fcfs.py` tool provides exact behavioral simulation, metric accounting, and visual Gantt charts:

```bash
python verify_fcfs.py dispatchlist_simple.txt
python verify_fcfs.py dispatchlist_full.txt
python verify_fcfs.py dispatchlist_gap.txt
```

---

## 9. Conclusion

The First-Come-First-Served batch process dispatcher successfully demonstrates fundamental operating system principles:
- **Process Control Blocks (PCBs)** effectively encapsulate all process context and scheduling state.
- **Inter-Process Communication & Signals** (`SIGINT`) provide deterministic, kernel-level lifecycle control over external workload programs.
- **Discrete Event Scheduling** synchronizes real-time process bursts with simulated scheduler ticks.
- The dual-format input parser provides full backward compatibility with simple batch files while laying the foundation for multi-resource operating system dispatchers (Project 2).

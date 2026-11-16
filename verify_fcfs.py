"""
FCFS Batch Process Dispatcher Simulator & Verification Tool
Operating Systems Coursework

Simulates the exact 5-step FCFS dispatcher algorithm from the assignment specification:
1. Initialize dispatcher queue;
2. Fill dispatcher queue from dispatch list file;
3. Start dispatcher timer;
4. While there's anything in the queue or there is a currently running process:
    i. If a process is currently running;
        a. Decrement process remainingcputime;
        b. If times up:
            A. Send SIGINT to the process to terminate it;
            B. Free up process structure memory
    ii. If no process currently running && dispatcher queue is not empty && arrivaltime of process at head of queue <= dispatcher timer:
        a. Dequeue process and start it (fork & exec)
        b. Set it as currently running process;
    iii. sleep for one second (simulated or real);
    iv. Increment dispatcher timer;
    v. Go back to 4.
5. Exit.
"""

import sys
import os
from dataclasses import dataclass
from typing import List, Optional

@dataclass
class PCB:
    job_id: int
    arrival_time: int
    burst_time: int
    remaining_cpu_time: int
    command: str
    args: List[str]
    priority: int = 0
    mem_alloc: int = 64
    pid: Optional[int] = None
    start_time: Optional[int] = None
    finish_time: Optional[int] = None

def parse_dispatch_file(filepath: str) -> List[PCB]:
    if not os.path.exists(filepath):
        print(f"Error: File '{filepath}' not found.")
        sys.exit(1)

    jobs: List[PCB] = []
    job_id = 1

    with open(filepath, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue

            parts = [p.strip() for p in line.split(",")]
            if len(parts) < 2:
                continue

            arrival = int(parts[0])

            if len(parts) >= 8:
                # Format 2: <arrival>,<priority>,<cputime>,<mem>,<r1>,<r2>,<r3>,<r4>
                priority = int(parts[1])
                burst = int(parts[2])
                mem = int(parts[3])
                cmd = "./process"
                args = ["./process", str(burst)]
                jobs.append(PCB(
                    job_id=job_id,
                    arrival_time=arrival,
                    burst_time=burst,
                    remaining_cpu_time=burst,
                    command=cmd,
                    args=args,
                    priority=priority,
                    mem_alloc=mem
                ))
            else:
                # Format 1: <arrival>,<command> [args...]
                cmd_parts = parts[1].split()
                cmd = cmd_parts[0] if cmd_parts else "sleepy"
                burst = int(cmd_parts[1]) if len(cmd_parts) > 1 and cmd_parts[1].isdigit() else 1
                jobs.append(PCB(
                    job_id=job_id,
                    arrival_time=arrival,
                    burst_time=burst,
                    remaining_cpu_time=burst,
                    command=cmd,
                    args=cmd_parts
                ))
            job_id += 1

    return jobs

def run_fcfs_simulation(filepath: str, step_sleep: float = 0.0):
    jobs = parse_dispatch_file(filepath)
    queue = list(jobs)
    completed_jobs: List[PCB] = []

    print("\n" + "="*80)
    print(f"   FCFS BATCH PROCESS DISPATCHER SIMULATION: {os.path.basename(filepath)}")
    print("="*80)
    print(f"Loaded {len(queue)} jobs into initial dispatcher queue:")
    print(f" {'Job':<6} | {'Arrival':<8} | {'Burst':<6} | {'Priority':<9} | {'Memory':<7} | {'Command'}")
    print("-" * 80)
    for j in queue:
        print(f" J{j.job_id:<5} | {j.arrival_time:<8} | {j.burst_time:<6} | {j.priority:<9} | {j.mem_alloc} MB   | {' '.join(j.args)}")
    print("-" * 80)

    # 3. Start dispatcher timer
    dispatcher_timer = 0
    current_process: Optional[PCB] = None
    timeline: List[str] = []

    print("\nStarting execution loop...")
    print("-" * 80)
    print(f" {'Time':<8} | {'Event / Action'}")
    print("-" * 80)

    # 4. While there's anything in the queue or there is a currently running process:
    while queue or (current_process is not None):
        # 4.i: If a process is currently running
        if current_process is not None:
            # 4.i.a: Decrement process remainingcputime
            current_process.remaining_cpu_time -= 1

            # 4.i.b: If times up:
            if current_process.remaining_cpu_time <= 0:
                current_process.finish_time = dispatcher_timer
                completed_jobs.append(current_process)
                print(f" [{dispatcher_timer:3d}s]    | [TERMINATION] J{current_process.job_id} ('{current_process.command}') finished burst.")
                print(f"           | --> Sent SIGINT to PID {current_process.pid}; freed PCB memory.")
                current_process = None
            else:
                print(f" [{dispatcher_timer:3d}s]    | [RUNNING] J{current_process.job_id} executing (Remaining: {current_process.remaining_cpu_time}s)")

        # 4.ii: If no process currently running && queue not empty && arrival <= timer
        if current_process is None:
            if not queue:
                # Both queue and running process are empty - finished!
                break
            head = queue[0]
            if head.arrival_time <= dispatcher_timer:
                current_process = queue.pop(0)
                current_process.pid = 1000 + current_process.job_id
                current_process.start_time = dispatcher_timer
                print(f" [{dispatcher_timer:3d}s]    | [START] Dequeued J{current_process.job_id} (Arrival={current_process.arrival_time}s, Burst={current_process.burst_time}s)")
                print(f"           | --> Started child process (Simulated PID {current_process.pid})")
            else:
                print(f" [{dispatcher_timer:3d}s]    | [IDLE] No job ready (Next arrival at {head.arrival_time}s)")

        if current_process:
            timeline.append(f"J{current_process.job_id}")
        else:
            timeline.append("IDLE")

        # 4.iii: sleep for one second
        if step_sleep > 0:
            import time
            time.sleep(step_sleep)

        # 4.iv: Increment dispatcher timer
        dispatcher_timer += 1

    print("-" * 80)
    print(f" [{dispatcher_timer:3d}s]    | [COMPLETION] Dispatcher queue empty and all processes finished.")
    print("="*80)

    # Performance Metrics Calculation
    print("\n" + "="*80)
    print("                     SCHEDULING PERFORMANCE METRICS")
    print("="*80)
    print(f" {'Job':<6} | {'Arrival':<8} | {'Burst':<6} | {'Start':<6} | {'Finish':<7} | {'Wait Time':<10} | {'Turnaround'}")
    print("-" * 80)

    total_wait = 0
    total_tat = 0
    total_burst = 0

    for j in completed_jobs:
        turnaround = j.finish_time - j.arrival_time
        waiting = turnaround - j.burst_time
        total_wait += waiting
        total_tat += turnaround
        total_burst += j.burst_time
        print(f" J{j.job_id:<5} | {j.arrival_time:<8} | {j.burst_time:<6} | {j.start_time:<6} | {j.finish_time:<7} | {waiting:<10} | {turnaround}")

    n = len(completed_jobs)
    avg_wait = total_wait / n if n else 0
    avg_tat = total_tat / n if n else 0
    cpu_util = (total_burst / dispatcher_timer) * 100 if dispatcher_timer > 0 else 100

    print("-" * 80)
    print(f" Total Simulation Time : {dispatcher_timer} seconds")
    print(f" Total Busy CPU Time   : {total_burst} seconds")
    print(f" CPU Utilization       : {cpu_util:.2f}%")
    print(f" Average Waiting Time  : {avg_wait:.2f} seconds")
    print(f" Average Turnaround    : {avg_tat:.2f} seconds")
    print("="*80)

    # Gantt Chart Visualization
    print("\nVisual Gantt Chart:")
    print("-" * (len(timeline) * 5 + 1))
    chart_bar = "|"
    for slot in timeline:
        chart_bar += f" {slot:^3}|"
    print(chart_bar)
    print("-" * (len(timeline) * 5 + 1))
    
    time_marks = "0"
    for t in range(1, len(timeline) + 1):
        time_marks += f"{t:>5}"
    print(time_marks)
    print()

if __name__ == "__main__":
    target_file = sys.argv[1] if len(sys.argv) > 1 else "dispatchlist_simple.txt"
    run_fcfs_simulation(target_file)

# Scientific-Paper-OS-UGD
Comprehensive Evaluation of FCFS, SJF (Pre-emptive &amp; Non-Pre-emptive) and Round-Robin Scheduling Algorithms on Modern Multicore Linux Systems

# Multicore Scheduler Evaluation

A simple C++ simulator and results for comparing FCFS, SJF (preemptive & non-preemptive) and Round-Robin on a 4-core model.

## Files

- `scheduler_4cores_metrics.cpp` — simulator source  
- `scheduler_4cores`              — compiled executable  
- `results.xlsx`                  — raw metrics (Excel)  
- `avg_waiting.png`               — Average Waiting Time  
- `avg_turnaround.png`            — Average Turnaround Time  
- `throughput.png`                — Throughput  
- `cpu_util.png`                  — CPU Utilization  
- `avg_response.png`              — Average Response Time  
- `ctx_switches.png`              — Context-Switch Count  
- `fairness.png`                  — Fairness Index  

## Build & Run

Compile with C++11 and optimizations:  
```bash
g++ -std=c++11 -O2 scheduler_4cores_metrics.cpp -o scheduler_4cores


// scheduler_4cores_metrics.cpp
#include <iostream>
#include <vector>
#include <deque>
#include <queue>
#include <algorithm>
#include <random>
#include <fstream>
#include <cmath>
#include <cstdio>

struct Process {
    int pid;
    int arrival;
    int burst;
};

struct Metrics {
    double avg_waiting;
    double avg_turnaround;
    double throughput;
    double cpu_util;
    double avg_response;
    int context_switches;
    double fairness;
};

// Genera N procesos con tiempos de llegada [0,MAX_ARR) y ráfagas [1,MAX_BURST]
std::vector<Process> gen_workload(int N, int MAX_ARR=1000, int MAX_BURST=200) {
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> darr(0, MAX_ARR);
    std::uniform_int_distribution<int> dburst(1, MAX_BURST);
    std::vector<Process> v; v.reserve(N);
    for(int i=0;i<N;i++){
        v.push_back(Process{i, darr(rng), dburst(rng)});
    }
    return v;
}

// Plantilla de simulación de tick a tick
Metrics simulate(
    const std::vector<Process>& procs,
    int M,
    const std::string& alg,
    int quantum=10
){
    int N = procs.size();
    // Estado de procesos
    std::vector<int> remaining(N), first_start(N, -1), finish(N, -1);
    for(int i=0; i<N; i++) remaining[i] = procs[i].burst;

    // Estadísticas
    std::vector<double> wait_times(N,0), response_times(N,0);
    int context_switches = 0;
    long long total_busy = 0;

    // Estructuras según algoritmo
    std::deque<int> fq;                        // FCFS & RR
    auto cmpBurst = [&](int a,int b){ return remaining[a] > remaining[b]; };
    std::priority_queue<int,std::vector<int>,decltype(cmpBurst)> sjf_pq(cmpBurst); // SRTF & SJF NP
    std::vector<int> cores(M, -1), quantum_left(M, quantum);

    int completed = 0;
    int next_arrival = 0;
    int currentTime = 0;

    while(completed < N) {
        // 1) Llegadas
        while(next_arrival < N && procs[next_arrival].arrival <= currentTime) {
            int pid = procs[next_arrival].pid;
            if(alg=="FCFS" || alg=="RR")
                fq.push_back(pid);
            else
                sjf_pq.push(pid);
            next_arrival++;
        }

        // 2) Posible preempción (solo SJF-P)
        if(alg=="SJF-P") {
            for(int c=0;c<M;c++){
                int p = cores[c];
                if(p<0) continue;
                if(!sjf_pq.empty() && remaining[sjf_pq.top()] < remaining[p]){
                    // preempte
                    fq.clear(); // no usado aquí
                    sjf_pq.push(p);
                    cores[c] = -1;
                    quantum_left[c] = quantum;
                    context_switches++;
                }
            }
        }
        // 3) Asignar cores libres
        for(int c=0;c<M;c++){
            if(cores[c]>=0) continue; // ocupado
            int pid = -1;
            if(alg=="FCFS" && !fq.empty()){
                pid = fq.front(); fq.pop_front();
            } else if(alg=="SJF-NP" || alg=="SJF-P"){
                if(!sjf_pq.empty()){
                    pid = sjf_pq.top(); sjf_pq.pop();
                }
            } else if(alg=="RR" && !fq.empty()){
                pid = fq.front(); fq.pop_front();
                quantum_left[c] = quantum;
            }
            if(pid>=0){
                cores[c] = pid;
                context_switches++;
                if(first_start[pid]<0){
                    first_start[pid] = currentTime;
                    response_times[pid] = currentTime - procs[pid].arrival;
                }
            }
        }

        // 4) Ejecutar 1 ms
        bool any = false;
        for(int c=0;c<M;c++){
            int p = cores[c];
            if(p>=0){
                any = true;
                remaining[p]--;
                total_busy++;
                if(alg=="RR"){
                    quantum_left[c]--;
                }
                // ¿terminó?
                if(remaining[p]==0){
                    finish[p] = currentTime+1;
                    wait_times[p] = (finish[p] - procs[p].arrival - procs[p].burst);
                    completed++;
                    cores[c] = -1;
                    context_switches++;
                }
                // ¿quantum expiró?
                else if(alg=="RR" && quantum_left[c]==0){
                    fq.push_back(p);
                    cores[c] = -1;
                    context_switches++;
                }
            }
        }
        // 5) Avanzar tiempo
        if(!any && next_arrival < N){
            currentTime = procs[next_arrival].arrival;
        } else {
            currentTime++;
        }
    }

    // 6) Calcular métricas
    double sumW=0, sumT=0, sumR=0;
    for(int i=0;i<N;i++){
        sumW += wait_times[i];
        sumT += (finish[i]-procs[i].arrival);
        sumR += response_times[i];
    }
    double avgW = sumW/N;
    double avgT = sumT/N;
    double avgR = sumR/N;
    double throughput = double(N) / currentTime;
    double cpuUtil = double(total_busy) / (double(M) * currentTime);
    // Fairness (Jain) sobre wait_times
    double sum = 0, sum2 = 0;
    for(int i=0;i<N;i++){
        sum += wait_times[i];
        sum2 += wait_times[i]*wait_times[i];
    }
    double fairness = (sum*sum) / (double(N) * sum2);

    return Metrics{avgW, avgT, throughput, cpuUtil, avgR, context_switches, fairness};
}

int main(){
    const int N = 100;
    const int M = 4;
    const int QUANTUM = 10;

    // 1) Generar workload y ordenar por llegada
    auto w = gen_workload(N);
    std::sort(w.begin(), w.end(),
        [](const Process&a,const Process&b){ return a.arrival < b.arrival; });

    // 2) Ejecutar cada algoritmo
    struct Rec{ std::string name; Metrics m; };
    std::vector<Rec> results;
    results.push_back({"FCFS",       simulate(w,M,"FCFS",QUANTUM)});
    results.push_back({"SJF-NP",     simulate(w,M,"SJF-NP",QUANTUM)});
    results.push_back({"SJF-P",      simulate(w,M,"SJF-P",QUANTUM)});
    results.push_back({"RR_Q10ms",   simulate(w,M,"RR",10)});
    results.push_back({"RR_Q5ms",    simulate(w,M,"RR",5)});
    results.push_back({"RR_Q20ms",   simulate(w,M,"RR",20)});

    // 3) Imprimir y guardar CSV
    std::ofstream out("results.csv");
    out << "Alg,AvgW,AvgT,Throughput,CPUUtil,AvgR,CSwitch,Fairness\n";
    std::cout<< "Alg,AvgW,AvgT,Throughput,CPUUtil,AvgR,CSwitch,Fairness\n";
    for(auto &r: results){
        auto &m=r.m;
        out << r.name << ","
            << m.avg_waiting << ","
            << m.avg_turnaround << ","
            << m.throughput << ","
            << m.cpu_util << ","
            << m.avg_response << ","
            << m.context_switches << ","
            << m.fairness << "\n";
        std::cout<< r.name << ","
            << m.avg_waiting << ","
            << m.avg_turnaround << ","
            << m.throughput << ","
            << m.cpu_util << ","
            << m.avg_response << ","
            << m.context_switches << ","
            << m.fairness << "\n";
    }
    out.close();

    // 4) Gnuplot sin ventanas (nox), genera PNG
    FILE *gp = popen("gnuplot -persist", "w");
    if(!gp){ std::cerr<<"Error lanzando gnuplot\n"; return 1; }
    fprintf(gp,
        "set datafile separator ','\n"
        "set style data histograms\n"
        "set style fill solid border -1\n"
        "set xtics rotate by -45\n"

        // AvgW
        "set title 'Average Waiting Time'\n"
        "set ylabel 'ms'\n"
        "set terminal png size 800,600\n"
        "set output 'avg_waiting.png'\n"
        "plot 'results.csv' using 2:xtic(1) notitle\n"

        // AvgT
        "set title 'Average Turnaround Time'\n"
        "set ylabel 'ms'\n"
        "set output 'avg_turnaround.png'\n"
        "plot 'results.csv' using 3:xtic(1) notitle\n"

        // Throughput
        "set title 'Throughput (proc/ms)'\n"
        "set ylabel 'throughput'\n"
        "set output 'throughput.png'\n"
        "plot 'results.csv' using 4:xtic(1) notitle\n"

        // CPU Util
        "set title 'CPU Utilization'\n"
        "set ylabel 'fraction'\n"
        "set output 'cpu_util.png'\n"
        "plot 'results.csv' using 5:xtic(1) notitle\n"

        // Avg Response
        "set title 'Average Response Time'\n"
        "set ylabel 'ms'\n"
        "set output 'avg_response.png'\n"
        "plot 'results.csv' using 6:xtic(1) notitle\n"

        // Context Switches
        "set title 'Context Switch Count'\n"
        "set ylabel '# switches'\n"
        "set output 'ctx_switches.png'\n"
        "plot 'results.csv' using 7:xtic(1) notitle\n"

        // Fairness
        "set title 'Fairness Index'\n"
        "set ylabel 'Jain Index'\n"
        "set output 'fairness.png'\n"
        "plot 'results.csv' using 8:xtic(1) notitle\n"
    );
    pclose(gp);

    std::cout<<"\nDone: metrics in results.csv and PNGs generated.\n";
    return 0;
}

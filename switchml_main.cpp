#include <iostream>
#include <vector>
#include <ranges>
#include <sstream>
#include <cstdint>
#include <algorithm>
#include <cctype>
#include <string>

#include "config.h"
#include "topology/hierarchical_topology.h"
#include "logfile.h"
#include "eventlist.h"

#include "job_submitter.h"
#include "job_scheduling/first_come_first_served.h"
#include "job_scheduling/fit_first.h"
#include "job_placement/random.h"
#include "job_placement/custom.h"
#include "job_placement/yarn.h"
#include "collective_scheduling/first_in_first_out_one_by_one.h"
#include "collective_scheduling/ready_and_go.h"
#include "collective_scheduling/bytescheduler.h"
#include "collective_scheduling/deficit_round_robin.h"

#include "cluster.h"
#include "job.h"
#include "csv.h"

typedef simtime_picosec SIM_UNIT;

int main() {

    simcpp20::simulation<SIM_UNIT> sim;
    auto event_list = EventList(sim);
    auto cluster = Cluster(event_list, SWITCH_PORTS, SWITCH_BUFFER, GPUS_PER_NODE);

    std::vector<Job *> jobs{};
    const char *value = getenv("JOB_CSV");
    // "../HeliosData/data/60_job.csv"
    if (value) {
        printf("JOB_CSV %s\n", value);
        io::CSVReader<5> in(value);
        in.read_header(io::ignore_missing_column, "num_gpu", "duration", "submit_time", "iterations", "model");
        unsigned num_gpu;
        simtime_picosec duration;
        simtime_picosec submit_time;
        unsigned iterations = 0;
        std::string model = "alexnet";
        unsigned counter = 0;
        while (in.read_row(num_gpu, duration, submit_time, iterations, model)) {
//            auto iters = duration * 1000 / 32;
//            if (counter < 16) {
//                counter++;
//            } else break;
            jobs.push_back(
                    new Job(timeFromMs(int(submit_time * 10)), sim, "alexnet", iterations > 9 ? iterations / 10 : 1,
                            num_gpu));
        }
    } else {
        printf("JOB_CSV SYNTHETIC\n");
//        jobs.push_back(new Job(0, sim, "alexnet", 5, 2));
//        jobs.push_back(new Job(0, sim, "alexnet", 5, 2));
        jobs.push_back(new Job(0, sim, std::vector<uint64_t>{2621440, 2621440}, 5, 2));
        jobs.push_back(new Job(0, sim, std::vector<uint64_t>{2621440, 2621440}, 5, 2));
    }

    unsigned cnt = 0;
    for (auto &job: jobs) {
        cnt += job->n_iter * job->model.size();
    }
    cpb.init_variable(cnt);
    cpb.cntSet(0);

    unsigned seed = std::stol(getenv("SEED", "0"));
    printf("SEED %u\n", seed);

    PlacementAlgo *placement_algo;
    auto placement_type = getenv("PLACEMENT", "yarn");
    std::transform(placement_type.begin(), placement_type.end(), placement_type.begin(), ::tolower);
    switch (hash_compile_time(placement_type.c_str())) {
        case "random"_hash:
            placement_algo = new RandomPlacement(seed);
            printf("PLACEMENT RandomPlacement\n");
            break;
        case "yarn_random"_hash:
            placement_algo = new YARNPlacement(seed, true);
            printf("PLACEMENT YARNPlacementWithFallbackToRandom\n");
            break;
        case "custom"_hash:
            placement_algo = new CustomPlacement();
            printf("PLACEMENT CustomPlacement\n");
            break;
        case "yarn"_hash:
        default:
            placement_algo = new YARNPlacement(seed);
            printf("PLACEMENT YARNPlacement\n");
            break;
    }
    cluster.set_placement_algo(placement_algo);

    SchedulingAlgo *scheduling_algo;
    auto job_scheduler_algo = getenv("JOB_SCHEDULER", "fcfs");
    std::transform(job_scheduler_algo.begin(), job_scheduler_algo.end(), job_scheduler_algo.begin(), ::tolower);
    switch (hash_compile_time(job_scheduler_algo.c_str())) {
        case "fitfirst"_hash:
        case "ff"_hash:
            scheduling_algo = new FitFirst();
            printf("JOB_SCHEDULER FitFirst\n");
            break;
        case "fcfs"_hash:
        default:
            scheduling_algo = new FirstComeFirstServed();
            printf("JOB_SCHEDULER FirstComeFirstServed\n");
            break;
    }

    CollectiveScheduler *cs = nullptr;
    auto collective_scheduler_algo = getenv("CS", "none");
    std::transform(collective_scheduler_algo.begin(), collective_scheduler_algo.end(),
                   collective_scheduler_algo.begin(), ::tolower);
    switch (hash_compile_time(collective_scheduler_algo.c_str())) {
        case "bytescheduler"_hash:
        case "bs"_hash:
            cs = new ByteScheduler(sim, cluster);
            printf("COLLECTIVE_SCHEDULER ByteScheduler\n");
            break;
        case "fifo"_hash:
        case "2"_hash:
            cs = new FirstInFirstOutOneByOne(sim, cluster);
            printf("COLLECTIVE_SCHEDULER FirstInFirstOutOneByOne\n");
            break;
        case "readygo"_hash:
        case "3"_hash:
            cs = new ReadyAndGo(sim, cluster);
            printf("COLLECTIVE_SCHEDULER ReadyAndGo\n");
            break;
        case "none"_hash:
        default:
            printf("COLLECTIVE_SCHEDULER None\n");
            break;
    }

    if (cs) cs->collective_scheduler(sim, cluster);
    cout << "==========================================================================\n";
    cout << cluster._topo->no_of_nodes() << " nodes in total. Each node has " << GPUS_PER_NODE << " GPUs" << endl;

    broker(sim, jobs, cluster);
    cluster_scheduler(sim, cluster, scheduling_algo, cs);
    sim.run();
//    sim.run_until(2e12);
    cout << "\nsimulation done at " << sim.now() << endl;
    delete cs;
    delete placement_algo;
    delete scheduling_algo;
    for (auto job:jobs) delete job;
    jobs.clear();
    return 0;
}

#include <iostream>
#include <vector>
#include <ranges>
#include <cstdint>
#include <cctype>
#include <string>
#include <csignal>
#include <glog/logging.h>

#include "config.h"
#include "eventlist.h"

#include "topology/hierarchical_topology.h"
#include "topology/custom_topology.h"
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
#include "collective_scheduling/sincronia.h"

#include "cluster.h"
#include "job.h"
#include "csv.h"
#include "common.h"

typedef simtime_picosec SIM_UNIT;

void signal_handler(int signal) {
    cpb.show_progress_bar();
}

int main(int argc, char *argv[]) {
    google::InitGoogleLogging("CloudSimCpp"); // without this, log to stderr
    google::InstallFailureSignalHandler();
    simcpp20::simulation<SIM_UNIT> sim;
    auto event_list = EventList(sim);
    auto cluster = Cluster(event_list);

    MyTopology *topo;
    auto topology = getenv("TOPOLOGY", "custom");
    std::transform(topology.begin(), topology.end(), topology.begin(), ::tolower);
    switch (hash_compile_time(topology.c_str())) {
        case "custom"_hash:
            topo = new CustomTopology(&cluster, SWITCH_PORTS, SWITCH_BUFFER, &event_list, GPUS_PER_NODE);
            printf("TOPOLOGY custom\n");
            break;
        case "hierarchical"_hash:
        default:
            topo = new HierarchicalTopology(&cluster, SWITCH_PORTS, SWITCH_BUFFER, &event_list, GPUS_PER_NODE);
            printf("TOPOLOGY hierarchical\n");
            break;
    }
    cluster.init_topo(topo);

    std::vector<Job *> jobs{};
    const char *value = getenv("JOB_CSV");
    if (value) {
        double shrink_iter_factor = std::stod(getenv("SHRINKITER", "1"));
        if (shrink_iter_factor != 1.) printf("SHRINKITER %f\n", shrink_iter_factor);
        double shrink_arrival_factor = std::stod(getenv("SHRINKARRIVAL", "1"));
        if (shrink_arrival_factor != 1.) printf("SHRINKARRIVAL %f\n", shrink_arrival_factor);
        unsigned gpu_scale_factor = std::stol(getenv("JOB_GPU_SCALE", "1"));
        if (gpu_scale_factor != 1) printf("JOB_GPU_SCALE %u\n", gpu_scale_factor);

        printf("JOB_CSV %s\n", value);
        io::CSVReader<5> in(value);
        in.read_header(io::ignore_missing_column, "num_gpu", "duration", "submit_time", "iterations", "model");
        unsigned num_gpu;
        unsigned duration;
        unsigned submit_time; // originally in seconds
        unsigned iterations = 0;
        std::string model = "resnet50";
        unsigned counter = 0;
        unsigned max_jobs = std::stol(getenv("MAX_JOBS", "4294967295"));
        if (max_jobs != 4294967295) printf("MAX_JOBS %u\n", max_jobs);

        while (in.read_row(num_gpu, duration, submit_time, iterations, model)) {
            if (counter < max_jobs) {
                counter++;
            } else break;
            auto iters = unsigned(iterations / shrink_iter_factor);
            if (iters == 0) iters = 1;
            jobs.push_back(new Job(timeFromSec(submit_time / shrink_arrival_factor), sim, model, iters,
                                   num_gpu * gpu_scale_factor));
        }
    } else {
        printf("JOB_CSV SYNTHETIC\n");
//        jobs.push_back(new Job(0, sim, "vgg19", 2, 8));
//        jobs.push_back(new Job(0, sim, "vgg19", 2, 8));
        jobs.push_back(new Job(0, sim, std::vector<uint64_t>{2621440, 2621440, 2621440}, 2, 8));
        jobs.push_back(new Job(0, sim, std::vector<uint64_t>{2621440, 2621440, 2621440}, 2, 8));
//        jobs.push_back(new Job(0, sim, std::vector<uint64_t>{2621440, 2621440}, 5, 8));
    }

    std::sort(jobs.begin(), jobs.end(), [](const auto &a, const auto &b) {
        return a->submit_time < b->submit_time;
    });

    unsigned cnt = 0;
    for (auto &job: jobs) {
//        cnt += (job->gpu > 1 ? job->n_iter * job->model.size() : 0);
        cnt += job->n_iter * job->model.size();
        // estimate how many collectives will be invoked, real value depends on job placement
    }
    cpb.init_variable(cnt);
    cpb.cntSet(0);
    std::signal(SIGUSR1, signal_handler);

    PlacementAlgo *placement_algo;
    auto placement_type = getenv("PLACEMENT", "yarn");
    std::transform(placement_type.begin(), placement_type.end(), placement_type.begin(), ::tolower);
    switch (hash_compile_time(placement_type.c_str())) {
        case "random"_hash:
            placement_algo = new RandomPlacement();
            printf("PLACEMENT RandomPlacement\n");
            break;
        case "random_distributed"_hash:
            placement_algo = new RandomPlacement(true);
            printf("PLACEMENT RandomPlacementForceDistributed\n");
            break;
        case "random_multi_rack"_hash:
            placement_algo = new RandomPlacement(true, true);
            printf("PLACEMENT RandomPlacementForceMultiRacks\n");
            break;
        case "yarn_random"_hash:
            placement_algo = new YARNPlacement(true);
            printf("PLACEMENT YARNPlacementWithFallbackToRandom\n");
            break;
//        case "custom"_hash:
//            placement_algo = new CustomPlacement();
//            printf("PLACEMENT CustomPlacement\n");
//            break;
        case "yarn"_hash:
        default:
            placement_algo = new YARNPlacement();
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
        case "onebyone"_hash:
            scheduling_algo = new FirstComeFirstServed(true);
            printf("JOB_SCHEDULER OneByOne\n");
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
        case "drr"_hash:
        case "4"_hash:
            cs = new DeficitRoundRobin(sim, cluster);
            printf("COLLECTIVE_SCHEDULER DeficitRoundRobin\n");
            break;
        case "sincronia"_hash:
        case "5"_hash:
            cs = new Sincronia(sim, cluster);
            printf("COLLECTIVE_SCHEDULER Sincronia\n");
            break;
        case "none"_hash:
        default:
            printf("COLLECTIVE_SCHEDULER None\n");
            break;
    }

    if (cs) cs->collective_scheduler(sim, cluster);
    auto submit_all_jobs = strtobool(getenv("SUBMIT_ALL_JOBS", "false"));
    if (submit_all_jobs) printf("SUBMIT_ALL_JOBS_IN_THE_BEGINNING true\n");
    broker(sim, jobs, cluster, submit_all_jobs);

    cout << "==========================================================================\n";
    cout << cluster._topo->no_of_nodes() << " nodes in total. Each node has " << GPUS_PER_NODE << " GPUs" << endl;

    cluster_scheduler(sim, cluster, scheduling_algo, cs);
    sim.run_until(1e19); // max is 18446744073709551615
//    for (auto worker: *cluster.workers) {
//        for (auto &pair: worker->tensors_for_job) {
//            for (auto tensor: pair.second) {
//                if (tensor->allreduced_size != tensor->size) {
//                    fmt::print("worker {} tensor {} size {} allreduced {} \n",
//                               worker->id, tensor->tensor_id, tensor->size, tensor->allreduced_size);
//                }
//            }
//        }
//    }
    delete cs;
    delete placement_algo;
    delete scheduling_algo;
    delete topo;
    for (auto job: jobs) delete job;
    jobs.clear();
    cout << "\nsimulation done at " << sim.now() << endl;
    std::clog << std::endl;
    return 0;
}

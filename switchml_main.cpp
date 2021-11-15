#include <iostream>
#include <vector>
#include <ranges>
#include <sstream>
#include <cstdint>

#include "config.h"
#include "topology/hierarchical_topology.h"
#include "logfile.h"
#include "eventlist.h"

#include "job_submitter.h"
#include "job_scheduling/first_come_first_served.h"
#include "job_placement/random.h"
#include "collective_scheduling/first_in_first_out_one_by_one.h"
#include "collective_scheduling/ready_and_go.h"
#include "collective_scheduling/bytescheduler.h"

#include "cluster.h"
#include "job.h"
#include "csv.h"

typedef simtime_picosec SIM_UNIT;

int main() {

    simcpp20::simulation<SIM_UNIT> sim;
    auto event_list = EventList(sim);
    auto cluster = Cluster(event_list, 8, SWITCH_BUFFER, 4);
    cout<<cluster._topo->no_of_nodes()<< " nodes in total. Each node has "<< GPUS_PER_NODE << " GPUs"<<endl;

    std::vector<Job *> jobs{};
    const char *value = getenv("JOB_CSV");
    // "../HeliosData/data/60_job.csv"
    if (value) {
        printf("using job file %s\n", value);
        io::CSVReader<5> in(value);
        in.read_header(io::ignore_extra_column, "num_gpu", "duration", "submit_time", "iterations", "model");
        int num_gpu;
        simtime_picosec duration;
        simtime_picosec submit_time;
        unsigned iterations;
        std::string model;
        while(in.read_row(num_gpu, duration, submit_time, iterations, model)){
            jobs.push_back(new Job(timeFromSec(submit_time), sim, "alexnet", iterations, num_gpu));
        }
    } else {
        printf("using hard coded jobs\n");
        jobs.push_back(new Job(0, sim, "alexnet", 5, 2));
        jobs.push_back(new Job(0, sim, std::vector<uint64_t>{2621440,2621440,2621440}, 5, 2));
    }

    unsigned cnt = 0;
    for (auto& job: jobs) {
        cnt += job->n_iter * job->model.size();
    }
    cpb.init_variable(cnt);
    cpb.cntSet(0);

    PlacementAlgo *placement_algo;
    SchedulingAlgo *scheduling_algo;
    RandomPlacement r = RandomPlacement(387);
    placement_algo = &r;
    FirstComeFirstServed f = FirstComeFirstServed();
    scheduling_algo = &f;

    CollectiveScheduler *cs = nullptr;
    const char *cs_type = getenv("CS");
    // "../HeliosData/data/60_job.csv"
    if (cs_type) {
        auto type = std::strtol(cs_type, nullptr, 10);
        switch(type) {
            case 1:
                cs = new ByteScheduler(sim, cluster);
                printf("scheduler: ByteScheduler\n");
                break;
            case 2:
                cs = new FirstInFirstOutOneByOne(sim, cluster);
                printf("scheduler: FirstInFirstOutOneByOne\n");
                break;
            case 3:
                cs = new ReadyAndGo(sim, cluster);
                printf("scheduler: ReadyAndGo\n");
                break;
            default:
                printf("scheduler: None\n");
        }
    } else printf("scheduler: None\n");
////    CollectiveScheduler *cs = nullptr;//new FirstInFirstOutOneByOne(sim, cluster);
////    CollectiveScheduler *cs = new ReadyAndGo(sim, cluster);
////    CollectiveScheduler *cs = new FirstInFirstOutOneByOne(sim, cluster);
//    CollectiveScheduler *cs = new ByteScheduler(sim, cluster);
    if (cs) cs->collective_scheduler(sim, cluster);

    broker(sim, jobs, cluster);
    cluster_scheduler(sim, cluster, scheduling_algo, placement_algo, cs);
    sim.run();
//    sim.run_until(2e12);
    cout << "\nsimulation done at " << sim.now() << endl;
    delete cs;
    for (auto job:jobs) delete job;
    jobs.clear();
    return 0;
}

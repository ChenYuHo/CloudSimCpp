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

//    io::CSVReader<5> in("../HeliosData/data/60_job.csv");
//    in.read_header(io::ignore_extra_column, "num_gpu", "duration", "submit_time", "iterations", "model");
//    int num_gpu;
//    simtime_picosec duration;
//    simtime_picosec submit_time;
//    unsigned iterations;
//    std::string model;
//    std::vector<Job *> jobs;
//    while(in.read_row(num_gpu, duration, submit_time, iterations, model)){
//        jobs.push_back(new Job(timeFromSec(submit_time), sim, "alexnet", iterations, num_gpu));
//    }


    auto jobs = std::vector<Job *>{
        new Job(0, sim, std::vector<uint64_t>{2621440,2621440,2621440}, 5, 2),
        new Job(0, sim, std::vector<uint64_t>{2621440,2621440,2621440}, 5, 2)
//            std::make_shared<Job>(0, sim, "alexnet", 3, 1),
//            std::make_shared<Job>(timeFromSec(30), sim, "alexnet", 5, 8),
//            std::make_shared<Job>(0, sim, "alexnet", 606, 1),
//            std::make_shared<Job>(30, sim, "alexnet", 30, 8),
//            new Job(30, sim,
//                    std::vector<uint64_t>{23232, 64, 307200, 192, 663552, 384, 884736, 256, 589824, 256, 37748736, 4096,
//                                          16777216, 4096, 4096000, 1000}, 30, 8),
            // layer size in number of elements!
//            new Job(0, sim, std::vector<uint64_t>{26214400}, 10, 2), 589824, 256, 37748736, 4096, 16777216, 4096, 4096000, 1000})
    };

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

    CollectiveScheduler *cs = nullptr;//new FirstInFirstOutOneByOne(sim, cluster);
//    CollectiveScheduler *cs = new ReadyAndGo(sim, cluster);
//    CollectiveScheduler *cs = new FirstInFirstOutOneByOne(sim, cluster);
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

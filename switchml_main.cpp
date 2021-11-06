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

#include "cluster.h"
#include "job.h"
#include "csv.h"

//uint32_t RTT = 1; // this is per link delay in us; identical RTT microseconds = 0.02 ms

typedef simtime_picosec SIM_UNIT;

//class TestEventSource : public EventSource {
//public:
//    TestEventSource(EventList &eventlist, const string &name) : EventSource(eventlist, name){};
//    void doNextEvent() override {
//        std::cout << "test event source " << eventlist().now() << std::endl;
//    }
//};

int main() {
    simcpp20::simulation<SIM_UNIT> sim;
    auto event_list = EventList(sim);
//    event_list.setEndtime(timeFromSec(2));
//    Logfile logfile("test.log", event_list);
//    TestEventSource t(event_list, "test");
//    event_list.sourceIsPending(t, 12325754841);

//    auto cluster = std::make_shared<Cluster>(event_list);
    auto cluster = Cluster(event_list, 2, SWITCH_BUFFER);
    cout<<cluster._topo->no_of_nodes()<< " nodes in total. Each node has "<< GPUS_PER_NODE << " GPUs"<<endl;


//    io::CSVReader<5> in("../HeliosData/data/60_job.csv");
//    in.read_header(io::ignore_extra_column, "num_gpu", "duration", "submit_time", "iterations", "model");
//    int num_gpu;
//    simtime_picosec duration;
//    simtime_picosec submit_time;
//    unsigned iterations;
//    std::string model;
//    std::vector<std::shared_ptr<Job>> jobs;
//    while(in.read_row(num_gpu, duration, submit_time, iterations, model)){
//        jobs.push_back(std::make_shared<Job>(timeFromSec(submit_time), sim, "alexnet", iterations, num_gpu));
//    }


    std::vector<std::shared_ptr<Job>> jobs = std::vector<std::shared_ptr<Job>>{
            std::make_shared<Job>(0, sim, std::vector<uint64_t>{100, 120}, 2, 2),
            std::make_shared<Job>(0, sim, std::vector<uint64_t>{90, 130}, 2, 2),
        // layer size in number of elements!
//            std::make_shared<Job>(0, sim, std::vector<uint64_t>{26214400}, 10, 2), 589824, 256, 37748736, 4096, 16777216, 4096, 4096000, 1000})
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
    if(cs) cs->collective_scheduler(sim, cluster);

    broker(sim, jobs, cluster);
    cluster_scheduler(sim, cluster, scheduling_algo, placement_algo, cs);

    sim.run();
//    sim.run_until(2e12);
    cout<<"\nsimulation done at "<<sim.now()<<endl;
    return 0;
}

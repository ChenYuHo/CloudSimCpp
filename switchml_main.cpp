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
#include "collective_scheduling/ready_and_go.h"

#include "cluster.h"
#include "job.h"

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
    auto cluster = Cluster(event_list);

    std::vector<std::shared_ptr<Job>> jobs = std::vector<std::shared_ptr<Job>>{
            std::make_shared<Job>(1e12, sim, std::vector<uint64_t>{200}),//16777216, 2000}),
            std::make_shared<Job>(1e12, sim,std::vector<uint64_t>{200})
//                                  std::vector<uint64_t>{1000}
//                                  std::vector<uint64_t>{25393, 23232, 64, 307200, 192, 663552, 384, 884736, 256,
//                                                        589824, 256, 37748736, 4096, 16777216, 4096, 4096000, 1000})
    };

    PlacementAlgo *placement_algo;
    SchedulingAlgo *scheduling_algo;
    RandomPlacement r = RandomPlacement();
    placement_algo = &r;
    FirstComeFirstServed f = FirstComeFirstServed();
    scheduling_algo = &f;

//    CollectiveScheduler *cs = nullptr;//new FirstInFirstOutOneByOne(sim, cluster);
    CollectiveScheduler *cs = new ReadyAndGo(sim, cluster);

    broker(sim, jobs, cluster);
    cluster_scheduler(sim, cluster, scheduling_algo, placement_algo, cs);

//    cluster.workers[0]->execute_job(sim, jobs[0], 2, nullptr);
//    cluster.workers[1]->execute_job(sim, jobs[0], 2, nullptr);
//    cluster.workers[0]->allreduce(1000, 2);
//    cluster.workers[1]->allreduce(1000, 2);
//    auto topology = (HierarchicalTopology*) cluster->topology();
//    auto tor_switch = std::make_shared<Switch>(event_list, cluster);
//    topology->register_switch(tor_switch.get());
//    auto worker1 = Worker(event_list, cluster, tor_switch);
//    auto worker2 = Worker(event_list, cluster, tor_switch);
//    topology->register_worker(&worker1);
//    topology->register_worker(&worker2);
//    worker1.eventlist().sourceIsPendingRel(worker1, 10);
//    worker2.eventlist().sourceIsPendingRel(worker2, 11);
//    worker.allreduce(0);
    // construct
//    auto jobs = std::vector<std::shared_ptr<Job>>{
//            std::make_shared<Job>(cluster, 1e12, event_list),
//            std::make_shared<Job>(cluster, 1e12, event_list, std::vector<uint64_t>{1000})
//    };

    // schedule job arrival events and start job scheduler
//    cluster->start_scheduler(jobs);


//
//    cluster.setup(conf);
//    PlacementAlgo *placement_algo;
//    SchedulingAlgo *scheduling_algo;
//    RandomPlacement r = RandomPlacement();
//    placement_algo = &r;
//    FirstComeFirstServed f = FirstComeFirstServed();
//    scheduling_algo = &f;
//
//    CollectiveScheduler *cs;
//    auto m = METHOD;
//    switch(str2int(METHOD))
//    {
//        case str2int("drr"):
//            cs = new DeficitRoundRobin();
//            break;
//        case str2int("bytescheduler"):
//            cs = new ByteScheduler();
//            break;
//        default:
//            cs = new FirstInFirstOutOneByOne();
//            break;
//    }

//    broker(sim, jobs, cluster);
//    cluster_scheduler(sim, cluster, scheduling_algo, placement_algo, cs);
//    cs->collective_scheduler(sim, cluster);
//    while (event_list.doNextEvent()) {
//    }
    sim.run();
//    sim.run_until(2e12);
//    delete placement_algo;
//    delete scheduling_algo;
    std::cout << "done" << std::endl;
    return 0;
}

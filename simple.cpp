#include <iostream>
#include <vector>
#include <ranges>
#include <cstdint>
#include <glog/logging.h>
#include "eventlist.h"
#include "topology/custom_topology.h"
#include "job_submitter.h"
#include "job_scheduling/first_come_first_served.h"
#include "job_placement/custom.h"
#include "cluster.h"
#include "job.h"
#include "common.h"

typedef simtime_picosec SIM_UNIT;

int main(int argc, char *argv[]) {
    google::InitGoogleLogging("simple"); // without this, log to stderr
    google::InstallFailureSignalHandler();
    simcpp20::simulation<SIM_UNIT> sim;
    auto event_list = EventList(sim);
    auto cluster = Cluster(event_list);

    MyTopology *topo = new CustomTopology(&cluster, SWITCH_PORTS, SWITCH_BUFFER, &event_list, GPUS_PER_NODE);
    cluster.init_topo(topo);

    std::vector<Job *> jobs{new Job(0, sim, std::vector<uint64_t>{26214400}, 1, 8)};
    cpb.init_variable(1);
    cpb.cntSet(0);
    PlacementAlgo *placement_algo = new CustomPlacement([](Cluster &cluster, Job *job){
        std::unordered_map<unsigned, unsigned> counter{};
        for (unsigned i=0; i<SWITCH_PORTS; ++i)
            counter[i] = 1;
        return counter;
    });
    cluster.set_placement_algo(placement_algo);

    SchedulingAlgo *scheduling_algo = new FirstComeFirstServed();
    CollectiveScheduler *cs = nullptr;
    broker(sim, jobs, cluster, false);

    cout << "==========================================================================\n";
    cout << cluster._topo->no_of_nodes() << " nodes in total. Each node has " << GPUS_PER_NODE << " GPUs" << endl;

    cluster_scheduler(sim, cluster, scheduling_algo, cs);
    sim.run_until(1e19); // max is 18446744073709551615
    cout << "\nsimulation done at " << sim.now() << endl;
    delete cs;
    delete placement_algo;
    delete scheduling_algo;
    delete topo;
    for (auto job: jobs) delete job;
    jobs.clear();
    std::clog << std::endl;
    return 0;
}

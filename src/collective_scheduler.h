#ifndef CLOUDSIMCPP_COLLECTIVE_SCHEDULER_H
#define CLOUDSIMCPP_COLLECTIVE_SCHEDULER_H

#include <memory>
#include "common.h"
#include <unordered_map>
#include <deque>
#include <queue>
#include <utility>

class Cluster;

class CollectiveScheduler {
public:
    CollectiveScheduler(simcpp20::simulation<SIM_UNIT> &sim, Cluster &cluster)
            : _sim(sim), cluster(cluster) {};

    virtual ~CollectiveScheduler() = default;

    // workers call whenever they want to do collective
    virtual simcpp20::event<SIM_UNIT> enqueue(simcpp20::simulation<SIM_UNIT> &,
                                              Tensor *) = 0;

    // server to spin up
    virtual simcpp20::event<SIM_UNIT> collective_scheduler(
            simcpp20::simulation<SIM_UNIT> &, Cluster &) = 0;

    // will be called after each job finishes
    virtual void cleanup_for_job(unsigned) {};

protected:
    simcpp20::simulation<SIM_UNIT> &_sim;
    Cluster &cluster;
};

#endif //CLOUDSIMCPP_COLLECTIVE_SCHEDULER_H

#ifndef CLOUDSIMCPP_READY_AND_GO_H
#define CLOUDSIMCPP_READY_AND_GO_H

#include "collective_scheduler.h"
#include "resource.hpp"


class ReadyAndGo : public CollectiveScheduler {
public:
    ReadyAndGo(simcpp20::simulation<SIM_UNIT> &sim, Cluster &cluster)
            : CollectiveScheduler(sim, cluster) {};
    std::unordered_map<uint64_t, std::deque<Tensor *>> queue;

    simcpp20::event<SIM_UNIT> enqueue(simcpp20::simulation<SIM_UNIT> &, Tensor *tensor) override;

    simcpp20::event<SIM_UNIT> collective_scheduler(simcpp20::simulation<SIM_UNIT> &sim, Cluster &cluster) override;

    std::unordered_map<unsigned, resource<SIM_UNIT>> job_locks{};
};



#endif //CLOUDSIMCPP_READY_AND_GO_H

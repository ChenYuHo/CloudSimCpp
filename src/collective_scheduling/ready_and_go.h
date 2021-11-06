#ifndef CLOUDSIMCPP_READY_AND_GO_H
#define CLOUDSIMCPP_READY_AND_GO_H

#include "collective_scheduler.h"


class ReadyAndGo : public CollectiveScheduler {
public:
    ReadyAndGo(simcpp20::simulation<SIM_UNIT> &sim, Cluster &cluster)
            : CollectiveScheduler(sim, cluster) {};
    std::unordered_map<std::pair<unsigned, unsigned>, std::deque<std::shared_ptr<Tensor>>, pair_hash> queue;

    simcpp20::event<SIM_UNIT> enqueue(simcpp20::simulation<SIM_UNIT> &, std::shared_ptr<Tensor> tensor) override;

    simcpp20::event<SIM_UNIT> collective_scheduler(simcpp20::simulation<SIM_UNIT> &sim, Cluster &cluster) override;
};



#endif //CLOUDSIMCPP_READY_AND_GO_H

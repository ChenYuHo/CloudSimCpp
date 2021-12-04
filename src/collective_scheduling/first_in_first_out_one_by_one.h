#ifndef CLOUDSIMCPP_FIRST_IN_FIRST_OUT_ONE_BY_ONE_H
#define CLOUDSIMCPP_FIRST_IN_FIRST_OUT_ONE_BY_ONE_H

#include "collective_scheduler.h"


class FirstInFirstOutOneByOne : public CollectiveScheduler {
public:
    FirstInFirstOutOneByOne(simcpp20::simulation<SIM_UNIT> &sim, Cluster &cluster)
            : CollectiveScheduler(sim, cluster) {};

    simcpp20::event<SIM_UNIT> collective_scheduler(simcpp20::simulation<SIM_UNIT> &sim, Cluster &cluster) override;

    resource<SIM_UNIT> lock{_sim, 1};

    simcpp20::event<SIM_UNIT> enqueue(simcpp20::simulation<SIM_UNIT> &sim, Tensor *tensor) override;

private:
    std::unordered_map<uint64_t, std::deque<Tensor *>> queue{};
};

#endif //CLOUDSIMCPP_FIRST_IN_FIRST_OUT_ONE_BY_ONE_H

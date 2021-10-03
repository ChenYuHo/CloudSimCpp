//
// Created by Chen-Yu Ho on 10/3/21.
//

#ifndef CLOUDSIMCPP_FIRST_IN_FIRST_OUT_ONE_BY_ONE_H
#define CLOUDSIMCPP_FIRST_IN_FIRST_OUT_ONE_BY_ONE_H

#include "collective_scheduler.h"


class FirstInFirstOutOneByOne : public CollectiveScheduler {
public:
    std::unordered_map<std::pair<unsigned, unsigned>, std::deque<std::shared_ptr<Tensor>>, pair_hash> queue;
    std::queue<std::deque<std::shared_ptr<Tensor>>> ready_queue;

    void enqueue(const std::shared_ptr<Tensor> &tensor) override;

    simcpp20::event<SIM_UNIT> collective_scheduler(simcpp20::simulation<SIM_UNIT> &sim, Cluster &cluster) override;
};



#endif //CLOUDSIMCPP_FIRST_IN_FIRST_OUT_ONE_BY_ONE_H

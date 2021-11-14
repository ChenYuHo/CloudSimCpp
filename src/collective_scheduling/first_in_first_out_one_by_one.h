//
// Created by Chen-Yu Ho on 10/3/21.
//

#ifndef CLOUDSIMCPP_FIRST_IN_FIRST_OUT_ONE_BY_ONE_H
#define CLOUDSIMCPP_FIRST_IN_FIRST_OUT_ONE_BY_ONE_H

#include "collective_scheduler.h"


class FirstInFirstOutOneByOne : public CollectiveScheduler {
public:
    FirstInFirstOutOneByOne(simcpp20::simulation<SIM_UNIT> &sim, Cluster &cluster)
            : CollectiveScheduler(sim, cluster) {};

    simcpp20::event<SIM_UNIT> collective_scheduler(simcpp20::simulation<SIM_UNIT> &sim, Cluster &cluster) override;

//    simcpp20::event<SIM_UNIT> run_scheduler_once(simcpp20::simulation<SIM_UNIT> &sim, Cluster &cluster);

    resource<SIM_UNIT> lock{_sim, 1};
};



#endif //CLOUDSIMCPP_FIRST_IN_FIRST_OUT_ONE_BY_ONE_H

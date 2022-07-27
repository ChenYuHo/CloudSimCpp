#ifndef CLOUDSIMCPP_ROUND_ROBIN_H
#define CLOUDSIMCPP_ROUND_ROBIN_H

#include "job_scheduler.h"

class RoundRobinPlacement : public PlacementAlgo {
public:
    std::unordered_map<unsigned, unsigned> place_job_in(Cluster &cluster, Job *job) override;
};

#endif //CLOUDSIMCPP_ROUND_ROBIN_H

#ifndef CLOUDSIMCPP_YARN_H
#define CLOUDSIMCPP_YARN_H

#include "job_scheduler.h"
#include <random>

class YARNPlacement : public PlacementAlgo {
public:
    explicit YARNPlacement(bool fallback_to_random = false)
            : use_random_when_fail(fallback_to_random) {};

    std::unordered_map<unsigned, unsigned> place_job_in(Cluster &cluster, Job *job) override;

private:
    bool use_random_when_fail;
};

#endif //CLOUDSIMCPP_YARN_H

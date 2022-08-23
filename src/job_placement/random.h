#ifndef CLOUDSIMCPP_RANDOM_H
#define CLOUDSIMCPP_RANDOM_H

#include "job_scheduler.h"
#include <random>

class RandomPlacement : public PlacementAlgo {
public:
    bool force_distributed;
    bool force_multi_racks;

    std::unordered_map<unsigned, unsigned> place_job_in(Cluster &cluster, Job *job) override;

    explicit RandomPlacement(bool force_distributed = false,
                             bool force_multi_racks = false) : force_distributed(force_distributed),
                                                               force_multi_racks(force_multi_racks) {};
};

#endif //CLOUDSIMCPP_RANDOM_H

#ifndef CLOUDSIMCPP_RANDOM_H
#define CLOUDSIMCPP_RANDOM_H

#include "job_scheduler.h"
#include <random>

class RandomPlacement : public PlacementAlgo {
public:
    std::random_device rd{};
    std::mt19937 gen;

    std::map<unsigned, unsigned> place_job_in(Cluster &cluster, Job *job) override;

    RandomPlacement():gen(std::mt19937(rd())){};
    explicit RandomPlacement(unsigned seed):gen(std::mt19937(seed)){};

};

#endif //CLOUDSIMCPP_RANDOM_H

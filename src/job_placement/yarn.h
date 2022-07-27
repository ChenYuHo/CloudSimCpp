#ifndef CLOUDSIMCPP_YARN_H
#define CLOUDSIMCPP_YARN_H

#include "job_scheduler.h"
#include <random>

class YARNPlacement : public PlacementAlgo {
public:
    std::random_device rd{};
    std::mt19937 gen;

    explicit YARNPlacement(bool fallback = false)
            : gen(std::mt19937(rd())), use_random_when_fail(fallback) {};

    explicit YARNPlacement(unsigned seed, bool fallback_to_random = false)
            : gen(std::mt19937(seed)), use_random_when_fail(fallback_to_random) {};

    std::unordered_map<unsigned, unsigned> place_job_in(Cluster &cluster, Job *job) override;

private:
    bool use_random_when_fail;
};

#endif //CLOUDSIMCPP_YARN_H

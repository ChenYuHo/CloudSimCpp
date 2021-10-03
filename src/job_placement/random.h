#ifndef CLOUDSIMCPP_RANDOM_H
#define CLOUDSIMCPP_RANDOM_H

#include "job_scheduler.h"
#include <random>

class RandomPlacement : public PlacementAlgo {
public:
    std::random_device rd{};
    std::mt19937 gen{rd()};

    std::unordered_map<unsigned, unsigned> place_job_in(
            Cluster &cluster, std::shared_ptr<Job> job) override;
};


//class RandomPlacement : public PlacementAlgo {
//public:
//    std::random_device rd{};
//    std::mt19937 gen{rd()};
//
//    std::unordered_map<unsigned, unsigned> place_job_in(
//            std::shared_ptr<Cluster>,
//            std::shared_ptr<Job>) override;
//};


#endif //CLOUDSIMCPP_RANDOM_H

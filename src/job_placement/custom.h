#ifndef CLOUDSIMCPP_CUSTOM_H
#define CLOUDSIMCPP_CUSTOM_H

#include <utility>

#include "job_scheduler.h"

class CustomPlacement : public PlacementAlgo {
public:
    explicit CustomPlacement(std::function<std::unordered_map<unsigned, unsigned>(Cluster &, Job *)> func)
            : func(std::move(func)) {};

    std::unordered_map<unsigned, unsigned> place_job_in(Cluster &cluster, Job *job) override;

private:
    std::function<std::unordered_map<unsigned, unsigned>(Cluster &, Job *)> func;
};

#endif // CLOUDSIMCPP_CUSTOM_H

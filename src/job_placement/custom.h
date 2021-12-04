#ifndef CLOUDSIMCPP_CUSTOM_H
#define CLOUDSIMCPP_CUSTOM_H

#include "job_scheduler.h"

class CustomPlacement : public PlacementAlgo {
public:
    std::map<unsigned, unsigned> place_job_in(Cluster &cluster, Job *job) override;
};

#endif // CLOUDSIMCPP_CUSTOM_H

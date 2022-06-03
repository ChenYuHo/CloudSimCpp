#ifndef CLOUDSIMCPP_ONE_BY_ONE_H
#define CLOUDSIMCPP_ONE_BY_ONE_H

#include "job_scheduler.h"

class OneByOne : public SchedulingAlgo {
public:
    Job* choose_job_to_execute_in(Cluster &cluster) override;
};

#endif //CLOUDSIMCPP_ONE_BY_ONE_H

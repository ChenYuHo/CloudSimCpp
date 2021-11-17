//
// Created by Chen-Yu Ho on 11/17/21.
//

#ifndef CLOUDSIMCPP_FIT_FIRST_H
#define CLOUDSIMCPP_FIT_FIRST_H

#include "job_scheduler.h"

class FitFirst : public SchedulingAlgo {
public:
    Job* choose_job_to_execute_in(Cluster &cluster) override;
};

#endif //CLOUDSIMCPP_FIT_FIRST_H

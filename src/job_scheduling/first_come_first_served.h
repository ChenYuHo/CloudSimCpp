#ifndef CLOUDSIMCPP_FIRST_COME_FIRST_SERVED_H
#define CLOUDSIMCPP_FIRST_COME_FIRST_SERVED_H

#include "job_scheduler.h"

class FirstComeFirstServed : public SchedulingAlgo {
public:
    Job* choose_job_to_execute_in(Cluster &cluster) override;
};

#endif //CLOUDSIMCPP_FIRST_COME_FIRST_SERVED_H

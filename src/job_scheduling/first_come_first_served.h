#ifndef CLOUDSIMCPP_FIRST_COME_FIRST_SERVED_H
#define CLOUDSIMCPP_FIRST_COME_FIRST_SERVED_H

#include "job_scheduler.h"

class FirstComeFirstServed : public SchedulingAlgo {
public:
    std::shared_ptr<Job> choose_job_to_execute_in(
            std::vector<std::shared_ptr<Job>>,
            std::shared_ptr<Cluster>) override;
};


#endif //CLOUDSIMCPP_FIRST_COME_FIRST_SERVED_H

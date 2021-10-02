//
// Created by Chen-Yu Ho on 9/29/21.
//

#include "first_come_first_served.h"
#include "job.h"

std::shared_ptr<Job> FirstComeFirstServed::choose_job_to_execute_in(
        std::vector<std::shared_ptr<Job>> jobs,
        std::shared_ptr<Cluster> cluster) {
    for (auto &job: jobs) {
        if (job->start_time == ULLONG_MAX) {
            return job;
        }
    }
    return nullptr;
}

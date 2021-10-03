//
// Created by Chen-Yu Ho on 9/29/21.
//

#include "first_come_first_served.h"
#include "cluster.h"
#include "job.h"

std::shared_ptr<Job> FirstComeFirstServed::choose_job_to_execute_in(Cluster &cluster) {
//        std::cout << "choose from " << cluster.jobs.size() << " ptr: " << &cluster << std::endl;
    for (auto &job: cluster.jobs) {
//            std::cout << "j " << job.start_time << std::endl;
        if (job->start_time == std::numeric_limits<uint64_t>::max()) {
            return job;
        }
    }
    return nullptr;
}



//std::shared_ptr<Job> FirstComeFirstServed::choose_job_to_execute_in(
//        std::vector<std::shared_ptr<Job>> jobs,
//        std::shared_ptr<Cluster> cluster) {
//    for (auto &job: jobs) {
//        if (job->start_time == ULLONG_MAX) {
//            return job;
//        }
//    }
//    return nullptr;
//}

#include "first_come_first_served.h"
#include "cluster.h"
#include "job.h"

Job* FirstComeFirstServed::choose_job_to_execute_in(Cluster &cluster) {
    for (auto &job: cluster.jobs) {
        if (job->start_time == (std::numeric_limits<uint64_t>::max)()) {
            return job;
        }
    }
    return nullptr;
}

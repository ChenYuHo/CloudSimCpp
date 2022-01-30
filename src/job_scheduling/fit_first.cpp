#include "fit_first.h"
#include "cluster.h"
#include "job.h"

Job* FitFirst::choose_job_to_execute_in(Cluster &cluster) {
    for (auto &job: cluster.jobs) {
        if (job->start_time == (std::numeric_limits<uint64_t>::max)()) {
            auto placement = cluster.placement->place_job_in(cluster, job);
            if (!placement.empty()) {
                return job;
            }
        }
    }
    return nullptr;
}

#include "one_by_one.h"
#include "cluster.h"
#include "job.h"

Job* OneByOne::choose_job_to_execute_in(Cluster &cluster) {
    for (auto &job: cluster.jobs) {
        if (job->start_time == std::numeric_limits<uint64_t>::max()) {
            return job;
        }
    }
    return nullptr;
}

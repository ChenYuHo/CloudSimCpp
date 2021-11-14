#include "common.h"
#include "cluster.h"
#include "job.h"

simcpp20::event<SIM_UNIT> broker(
        simcpp20::simulation<SIM_UNIT> &sim,
        std::vector<Job *> &jobs,
        Cluster &cluster) {
    for (auto &job:jobs) {
        co_await sim.timeout(job->submit_time - sim.now());
        myprintf("[%llu]\tjob %u arrived\n", sim.now(), job->id);
        cluster.add_job(job);
    }
    cluster.set_all_jobs_submitted();
}

#include "common.h"
#include "cluster.h"
#include "job.h"

simcpp20::event<SIM_UNIT> broker(
        simcpp20::simulation<SIM_UNIT> &sim,
        std::vector<Job *> &jobs,
        Cluster &cluster,
        bool submit_all_jobs_in_the_beginning) {
    for (auto &job:jobs) {
        if (!submit_all_jobs_in_the_beginning) {
            co_await sim.timeout(job->submit_time - sim.now());
        }
        myprintf(3, "[%llu]\tjob %u arrived, n_iter: %u\n", sim.now(), job->id, job->n_iter);
        cluster.add_job(job);
    }
    cluster.set_all_jobs_submitted();
    co_await sim.timeout(0);
}

#include "common.h"
#include "job_scheduler.h"
#include "job.h"
#include "cluster.h"
#include "worker.h"
#include "config.h"


simcpp20::event<SIM_UNIT>
cluster_scheduler(simcpp20::simulation<SIM_UNIT> &sim,
                  Cluster &cluster,
                  SchedulingAlgo *s,
                  CollectiveScheduler *cs) {
    while (!cluster.all_jobs_started) {
        co_await sim.timeout(timeFromSec(0));
        auto job = s->choose_job_to_execute_in(cluster);
        if (job != nullptr) {
            myprintf(0, "[%lu]\tjob %d which requires %d gpus is chosen\n", sim.now(), job->id, job->gpu);
            auto run_config = cluster.placement->place_job_in(cluster, job);
            if (run_config.empty()) {
                if (cluster.num_running_jobs() == 0) {
                    myprintf(0, "Job %d cannot be placed in the cluster.\n", job->id);
                    exit(1);
                }
            } else {
                auto str = fmt::format("[{}]\tjob {} placement: ", sim.now(), job->id);
                job->num_workers_allocated = run_config.size();
                for (auto const& it: run_config) {
                    job->wids_allocated.insert(it.first);
                }
                // multiple GPUs in one machine count as 1, assumming local reduce/broadcast is implicitly handled
                job->master_mid = run_config.begin()->first;
                // sets num_updates_for_job and downward_ids_for_job
                cluster._topo->set_switch_num_updates(job->id, run_config);
                job->start_time = sim.now();
                cluster.check_if_all_jobs_started();
                unsigned rank = 0;
                for (const auto &pair: run_config) {
                    str += fmt::format("mid {} rank {} -> {} gpu, ", pair.first, rank, pair.second);
                    cluster.worker_map[pair.first]->rank_for_job[job->id] = rank++;
                    cluster.worker_map[pair.first]->execute_job(sim, job, pair.second, cs);
                }
                str.pop_back(); // safe to do since run_config is not empty
                str.pop_back();
                myprintf(3, "%s\n", str.c_str());
                sim.timeout(0);
                continue;
            }
        }
        co_await sim.timeout(timeFromSec(1));
    }
}

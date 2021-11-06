#include "common.h"
#include "job_scheduler.h"
#include "job.h"
#include "cluster.h"
#include "worker.h"
#include "config.h"
#include "topology.h"


simcpp20::event<SIM_UNIT>
cluster_scheduler(simcpp20::simulation<SIM_UNIT> &sim,
                  Cluster &cluster,
                  SchedulingAlgo *s,
                  PlacementAlgo *p,
                  CollectiveScheduler *cs) {
    while (!cluster.all_jobs_started) {
        std::shared_ptr<Job> job = s->choose_job_to_execute_in(cluster);
        if (job != nullptr) {
            myprintf("[%llu]\tjob %d which requires %d gpus is chosen\n", sim.now(), job->id, job->gpu);
            auto run_config = p->place_job_in(cluster, job);
            if (run_config.empty()) {
                myprintf("[%llu]\tplacement failed for task %d requiring %d GPUs\n", sim.now(), job->id, job->gpu);
            } else {
                myprintf("[%llu]\tjob %d placement: ", sim.now(), job->id);
                job->num_workers_allocated = run_config.size();
                // multiple GPUs in one machine count as 1, assumming local reduce/broadcast is implicitly handled
                job->master_mid = run_config.begin()->first;
                // sets num_updates_for_job and downward_ids_for_job
                cluster._topo->set_switch_num_updates(job->id, run_config);
                job->start_time = sim.now();
                cluster.check_if_all_jobs_started();
                unsigned rank = 0;
                for (const auto &pair: run_config) {
                    myprintf("mid %d rank %u -> %d gpu ", pair.first, rank, pair.second);
                    cluster.worker_map[pair.first]->rank_for_job[job->id] = rank;
                    cluster.worker_map[pair.first]->execute_job(sim, job, pair.second, cs);
                    ++rank;
                }
                myprintf("\n");
            }
        }
        co_await sim.timeout(timeFromSec(1));
    }
}

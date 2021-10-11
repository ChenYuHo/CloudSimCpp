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
            printf("[%llu]\tjob %d which requires %d gpus is chosen\n", sim.now(), job->id, job->gpu);
            auto run_config = p->place_job_in(cluster, job);
            if (run_config.empty()) {
                printf("[%llu]\tplacement failed for task %d requiring %d GPUs\n", sim.now(), job->id, job->gpu);
            } else {
                printf("[%llu]\tjob %d placement: ", sim.now(), job->id);
                job->num_workers_allocated = run_config.size(); // multiple GPUs in one machine count as 1
                for (const auto &pair: run_config) {
                    printf("mid %d -> %d gpu ", pair.first, pair.second);
                    cluster.worker_map[pair.first]->execute_job(sim, job, pair.second, cs);
                }
                printf("\n");
                // sets num_updates_for_job and downward_ids_for_job
                cluster._topo->set_switch_num_updates(job->id, run_config);
            }
            co_await sim.timeout(0);
            continue; // There could be multiple jobs with the same submission timestamp
        }
        co_await sim.timeout(timeFromSec(1));
    }
}

//
// Created by Chen-Yu Ho on 9/27/21.
//

//#include <algorithm>
//#include <utility>
//#include "job_scheduler.h"
//#include "cluster.h"
//#include "job_scheduling/first_come_first_served.h"
//#include "job_placement/random.h"
//#include "job_submitter.h"
//#include "config.h"
//
//typedef std::uint64_t hash_t;
//constexpr hash_t prime = 0x100000001B3ull;
//constexpr hash_t basis = 0xCBF29CE484222325ull;
//
//constexpr hash_t hash_(char const *str, hash_t last_value = basis) {
//    return *str ? hash_(str + 1, (*str ^ last_value) * prime) : last_value;
//}
//
//constexpr uint64_t operator "" _hash(char const *p, size_t) {
//    return hash_(p);
//}
//
//JobScheduler::JobScheduler(EventList &ev, const shared_ptr<Cluster> &cluster,
//                           vector<shared_ptr<Job>> jobs,
//                           const std::string &scheduling_algo,
//                           const std::string &placement_algo)
//        : EventSource(ev, "JobScheduler"),
//          _cluster(cluster) {
//    job_submitter = make_shared<JobSubmitter>(ev, std::move(jobs), this);
//    switch (hash_(scheduling_algo.c_str())) {
//        case "fcfs"_hash:
//            _scheduling = make_shared<FirstComeFirstServed>();
//            break;
//        default:
//            cout << "Invalid scheduling algorithm: " << scheduling_algo << endl;
//            assert(0);
//    }
//
//    switch (hash_(placement_algo.c_str())) {
//        case "random"_hash:
//            _placement = make_shared<RandomPlacement>();
//            break;
//        default:
//            cout << "Invalid placement algorithm: " << scheduling_algo << endl;
//            assert(0);
//    }
//}
//
//void JobScheduler::start_scheduler() {
//    auto first_job_arrival_time = job_submitter->start_submitting_jobs();
//    if (first_job_arrival_time < UINT64_MAX)
//        eventlist().sourceIsPending(*this, first_job_arrival_time);
//    else _all_jobs_started = true;
//}
//
//void JobScheduler::doNextEvent() {
//    if (_all_jobs_started) {
//        printf("[%llu]\tAll jobs started, quitting scheduler\n", eventlist().now());
//        return;
//    }
//
//    std::shared_ptr<Job> job = _scheduling->choose_job_to_execute_in(_jobs, _cluster);
//    if (job != nullptr) {
//        printf("[%llu]\tjob %d which requires %d gpus is chosen\n", eventlist().now(), job->id(),
//               job->gpus_required);
//        auto run_config = _placement->place_job_in(_cluster, job);
//        if (run_config.empty()) {
//            printf("[%llu]\tplacement failed for task %d requiring %d GPUs\n", eventlist().now(), job->id(),
//                   job->gpus_required);
//        } else {
//            printf("[%llu]\tjob %d placement: ", eventlist().now(), job->id());
//            job->num_workers_allocated = run_config.size();
//            job->start_time = eventlist().now();
//            check_if_all_jobs_are_started();
//            for (const auto &pair: run_config) {
//                printf("mid %d -> %d gpu ", pair.first, pair.second);
//                _cluster->workers()[pair.first]->execute_job(job, pair.second);
//            }
//            printf("\n");
//        }
//        // There could be multiple jobs with the same submission timestamp
//        eventlist().sourceIsPendingRel(*this, 0);
//        return;
//    }
//    eventlist().sourceIsPendingRel(*this, timeFromSec(1));
//}
//
//void JobScheduler::add_pending_job(const std::shared_ptr<Job> &job) {
//    _jobs.push_back(job);
//}
//
//bool JobScheduler::all_jobs_submitted() {
//    return job_submitter->all_jobs_submitted();
//}
//
//void JobScheduler::check_if_all_jobs_are_started() {
//    if (!all_jobs_submitted())
//        return;
//    for (const auto &job: _jobs) {
//        if (job->start_time == ULLONG_MAX)
//            return;
//    }
//    _all_jobs_started = true;
//}


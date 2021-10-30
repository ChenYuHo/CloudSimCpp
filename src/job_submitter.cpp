#include "common.h"
#include "cluster.h"
#include "job.h"

simcpp20::event<SIM_UNIT> broker(
        simcpp20::simulation<SIM_UNIT> &sim,
        std::vector<std::shared_ptr<Job>> &jobs,
        Cluster &cluster) {
    for (auto &job:jobs) {
        co_await sim.timeout(job->submit_time - sim.now());
//        job->submitted_time = sim.now();
        myprintf("[%llu]\tjob %u arrived\n", sim.now(), job->id);
        cluster.jobs.push_back(job);
    }
    cluster.all_jobs_submitted = true;
}

////
//// Created by Chen-Yu Ho on 9/30/21.
////
//
//#include "job_submitter.h"
//
//#include "job.h"
//#include "job_scheduler.h"
//
//simtime_picosec JobSubmitter::start_submitting_jobs() {
//    if (!_jobs.empty()) {
//        auto job = _jobs.front();
//        eventlist().sourceIsPending(*this, job->submit_time);
//        return job->submit_time;
//    } else {
//        _all_jobs_submitted = true;
//        return UINT64_MAX;
//    }
//}
//
//void JobSubmitter::doNextEvent() {
//    auto job = _jobs.front();
//    _scheduler->add_pending_job(job);
////    eventlist().sourceIsPending(*_scheduler, job->submit_time);
//    myprintf("[%llu] job %u submitted at %llu\n", eventlist().now(), job->id(), job->submit_time);
//    _jobs.erase(_jobs.begin());
//    if (!_jobs.empty()) {
//        auto next_job = _jobs.front();
//        eventlist().sourceIsPending(*this, next_job->submit_time);
//    } else _all_jobs_submitted = true;
//}

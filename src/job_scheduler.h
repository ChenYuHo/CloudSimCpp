//
// Created by Chen-Yu Ho on 9/27/21.
//

#ifndef CLOUDSIMCPP_JOB_SCHEDULER_H
#define CLOUDSIMCPP_JOB_SCHEDULER_H

//#include <memory>
//#include <unordered_map>
//#include <random>
//#include <utility>
//#include "eventlist.h"
#include "collective_scheduler.h"

class Job;

//class JobSubmitter;

class Cluster;

class SchedulingAlgo {
public:
    virtual std::shared_ptr<Job> choose_job_to_execute_in(Cluster &cluster) = 0;
};

class PlacementAlgo {
public:
    virtual std::unordered_map<unsigned, unsigned> place_job_in(
            Cluster &, std::shared_ptr<Job>) = 0;
};

simcpp20::event<SIM_UNIT>
cluster_scheduler(simcpp20::simulation<SIM_UNIT> &sim,
                  Cluster &cluster,
                  SchedulingAlgo *s,
                  PlacementAlgo *p,
                  CollectiveScheduler *cs);

//class JobScheduler : public EventSource {
//public:
//    JobScheduler(EventList &ev, const shared_ptr<Cluster> &,
//                 vector<shared_ptr<Job>>, const std::string&, const std::string&);
//
//    bool all_jobs_started() { return _all_jobs_started; };
//
//    bool all_jobs_submitted();
//
//    void add_pending_job(const std::shared_ptr<Job> &);
//
//    void start_scheduler();
//
//private:
//    bool _all_jobs_started{false};
//    std::shared_ptr<Cluster> _cluster;
//    std::shared_ptr<JobSubmitter> job_submitter;
//    std::vector<std::shared_ptr<Job>> _jobs{};
//    void check_if_all_jobs_are_started();
//
//    void doNextEvent() override;
//
//    std::shared_ptr<SchedulingAlgo> _scheduling;
//    std::shared_ptr<PlacementAlgo> _placement;
//};

#endif //CLOUDSIMCPP_JOB_SCHEDULER_H

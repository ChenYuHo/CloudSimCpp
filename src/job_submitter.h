#ifndef CLOUDSIMCPP_JOB_SUBMITTER_H
#define CLOUDSIMCPP_JOB_SUBMITTER_H

simcpp20::event<SIM_UNIT> broker(
        simcpp20::simulation<SIM_UNIT> &sim,
        std::vector<std::shared_ptr<Job>> &jobs,
        Cluster &cluster);
//
//#include <utility>
//#include "eventlist.h"
//#include "job.h"
//
//class Job;
//
//class JobScheduler;
//
//class JobSubmitter : public EventSource {
//private:
//    std::vector<std::shared_ptr<Job>> _jobs{};
//    bool _all_jobs_submitted{false};
//    JobScheduler* _scheduler{};
//public:
//    bool all_jobs_submitted() { return _all_jobs_submitted; };
//
//    JobSubmitter(EventList &event_list, JobScheduler* js)
//            : EventSource(event_list, "JobSubmitter"), _scheduler(js) {};
//
//    JobSubmitter(EventList &event_list,
//                 std::vector<std::shared_ptr<Job>> jobs,
//                 JobScheduler* js)
//            : EventSource(event_list, "JobSubmitter"),
//              _jobs(std::move(jobs)),
//              _scheduler(js) {};
//
//    simtime_picosec start_submitting_jobs();
//
//    void doNextEvent() override;
//};
//
//
#endif //CLOUDSIMCPP_JOB_SUBMITTER_H

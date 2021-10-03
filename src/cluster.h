#ifndef CLOUDSIMCPP_CLUSTER_H
#define CLOUDSIMCPP_CLUSTER_H
#include "common.h"

class Switch;
class Worker;

class Cluster {
public:
    std::vector<std::shared_ptr<Worker>> machines;
    std::unordered_map<unsigned, std::shared_ptr<Worker>> machine_map;
    std::vector<std::shared_ptr<Switch>> tors;
    std::vector<std::shared_ptr<Job>> jobs;
    bool all_jobs_submitted{};
    bool all_jobs_started{};
    bool all_jobs_finished{};

    void add_job(const std::shared_ptr<Job> &job) {
        jobs.push_back(job);
    }

    void setup(config conf);

    void check_if_all_jobs_started();

    void check_if_all_jobs_finished();

};

//
//#include "topology.h"
//#include "worker.h"
//#include "switch.h"
//#include "job.h"
//#include "job_scheduler.h"
//
//class Job;
//
//class Cluster : EventSource {
//public:
//    Cluster(EventList &event_list) : EventSource(event_list, "Cluster") {
//        init_topo();
//    };
//
//    void init_topo();
//
//    void doNextEvent() override;
//
////    std::vector<shared_ptr<Job>> pending_jobs() { return _pending_jobs;};
//
//    std::vector<std::shared_ptr<Worker>> workers() {return _workers;};
//
//    Topology* topology(){return _topo;};
//private:
//    Topology* _topo;
//    std::vector<std::shared_ptr<Worker>> _workers{};
//    std::unordered_map<unsigned, std::shared_ptr<Worker>> machine_map{};
//    std::vector<std::shared_ptr<Switch>> _switches{};
//    std::vector<std::shared_ptr<Job>> _jobs;
//
//    bool all_jobs_submitted{};
//    bool all_jobs_started{};
//    bool all_jobs_finished{};
//
//};
//
//
#endif //CLOUDSIMCPP_CLUSTER_H

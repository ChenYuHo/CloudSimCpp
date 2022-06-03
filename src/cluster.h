#ifndef CLOUDSIMCPP_CLUSTER_H
#define CLOUDSIMCPP_CLUSTER_H
#include "common.h"
#include "eventlist.h"
#include "job_scheduler.h"
#include "topology/mytopology.h"

class Switch;
class Worker;

class Cluster : EventSource {
public:
    std::vector<Worker *> *workers{};
    std::unordered_map<unsigned, Worker *> worker_map{};
    std::vector<Switch *> *switches{}; // ToRs
    std::unordered_map<unsigned, Switch *> switch_map{};
    std::vector<Job *> jobs{};
    bool all_jobs_submitted{};
    bool all_jobs_started{};
    bool all_jobs_finished{};
    MyTopology *_topo{};
    PlacementAlgo *placement{};

    void set_placement_algo(PlacementAlgo *algo) { placement = algo; }

    void set_all_jobs_submitted() { all_jobs_submitted = true; }

    explicit Cluster(EventList &event_list) :
            EventSource(event_list, "Cluster") {};

    void init_topo(MyTopology *);

    void add_job(Job *job) {
        jobs.push_back(job);
    }

    void check_if_all_jobs_started();

    void check_if_all_jobs_finished();

    unsigned num_running_jobs() const;

private:
    void doNextEvent() override {};
};

#endif //CLOUDSIMCPP_CLUSTER_H

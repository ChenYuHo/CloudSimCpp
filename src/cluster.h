#ifndef CLOUDSIMCPP_CLUSTER_H
#define CLOUDSIMCPP_CLUSTER_H
#include "common.h"
#include "topology.h"
#include "eventlist.h"
#include "job_scheduler.h"

class Switch;
class Worker;

class Cluster : EventSource {
public:
    std::vector<Worker *> workers;
    std::unordered_map<unsigned, Worker *> worker_map{};
    std::vector<Switch *> switches{}; // ToRs
    std::unordered_map<unsigned, Switch *> switch_map{};
    std::vector<Job *> jobs{};
    bool all_jobs_submitted{};
    bool all_jobs_started{};
    bool all_jobs_finished{};
    Topology *_topo{};
    PlacementAlgo *placement{};

    void set_placement_algo(PlacementAlgo *algo) { placement = algo; }

    void set_all_jobs_submitted() { all_jobs_submitted = true; }

    explicit Cluster(EventList &event_list, unsigned switch_ports = SWITCH_PORTS, mem_b switch_buffer = SWITCH_BUFFER,
                     unsigned gpus_per_node = GPUS_PER_NODE) :
            EventSource(event_list, "Cluster") {
        init_topo(switch_ports, switch_buffer, gpus_per_node);
    };

    ~Cluster() override;

    void init_topo(int, mem_b, unsigned);

    void add_job(Job *job) {
        jobs.push_back(job);
    }

    void check_if_all_jobs_started();

    void check_if_all_jobs_finished();

private:
    void doNextEvent() override {};
};

#endif //CLOUDSIMCPP_CLUSTER_H

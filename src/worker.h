#ifndef CLOUDSIMCPP_WORKER_H
#define CLOUDSIMCPP_WORKER_H

#include "resource.hpp"
#include "common.h"
#include "collective_scheduler.h"
#include "eventlist.h"
#include "network.h"
//#include "switch.h"

class Job;

class Switch;

class Cluster;

//struct pkt;
//struct config;


class Worker : public EventSource, public PacketSink {
private:
    static unsigned get_id() {
        static unsigned ID = 0;
        return ID++;
    }

public:
    unsigned id;
    unsigned gpu;
    unsigned gpu_capacity;
    std::vector<Job *> jobs;
    Cluster *cluster;
    Switch *tor;

    void doNextEvent() override;

    void receivePacket(Packet &) override;

    const string &nodename() override {
        static string example = "Worker";
        return example;
    };

    explicit Worker(EventList &ev, Cluster *cluster, Switch *tor, unsigned gpus_per_node) :
            EventSource(ev, "worker"),
            id(get_id()), cluster(cluster),
            tor(tor), gpu(gpus_per_node), gpu_capacity(gpus_per_node) {
//        myprintf("Worker %d constructor invoked\n", id);
    }

    //TODO: model forward pass
    static simtime_picosec forward_pass_time(const uint64_t &size) {
        return size * 50;
    }

    //TODO: model backward pass
    static simtime_picosec backward_pass_time(const uint64_t &size) {
        return size * 50;
    }

    simcpp20::event<SIM_UNIT>
    execute_job(simcpp20::simulation<SIM_UNIT> &, Job *, unsigned, CollectiveScheduler *);

    simcpp20::event<SIM_UNIT> allreduce(simcpp20::simulation<SIM_UNIT> &, Tensor *, unsigned= 0);

    void sendPacket(unsigned, unsigned, unsigned, unsigned, Tensor *);

    std::unordered_map<unsigned, unsigned> rank_for_job{};
    std::unordered_map<unsigned, bool> clean_ToR_for_job{};

    std::unordered_map<uint64_t, resource<SIM_UNIT> *> fp_locks{};
    std::unordered_map<uint64_t, resource<SIM_UNIT> *> allreduce_locks{};
    std::unordered_map<unsigned, std::set<unsigned>> received_pkts{};
    unsigned allreduce_counter[2]{0, 0};
};

#endif //CLOUDSIMCPP_WORKER_H

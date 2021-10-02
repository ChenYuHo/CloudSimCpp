#ifndef CLOUDSIMCPP_WORKER_H
#define CLOUDSIMCPP_WORKER_H

#include <cstdint>
#include <vector>
#include <memory>
#include "switch.h"
#include "packet.h"
#include "cluster.h"
#include "config.h"
#include "eventlist.h"

//class Cluster;

class Job;

class Worker : public EventSource, public PacketSink {
public:
    unsigned num_free_gpus() const { return _gpu; }

    unsigned id() const { return _id; }

//    simcpp20::simulation<SIM_UNIT> sim{};

    explicit Worker(EventList &ev, Cluster* cluster, std::shared_ptr<Switch> tor) :
            EventSource(ev, "worker"),
            _id(get_id()), cluster(cluster),
            _tor(std::move(tor)){
//        sim = ev.sim();
        printf("Machine %d constructor invoked\n", _id);
    }

    explicit Worker(unsigned g_per_m, EventList &ev, Cluster *cluster, Switch *tor) :
            EventSource(ev, "worker"),
            _id(get_id()), _gpu(g_per_m),
            _gpu_capacity(g_per_m), cluster(cluster),
            _tor(tor) {
//        sim = ev.sim();
        printf("Machine %d constructor invoked\n", _id);
    }

    void doNextEvent() override;

    void receivePacket(Packet &) override;

    const string & nodename() override {
        static string example = "WORKER";
        return example;
    };

    void allreduce(uint64_t);

    void execute_job(const std::shared_ptr<Job>& job, const unsigned gpus_required);

    //TODO: model forward pass
//    static simtime_picosec forward_pass_time(const uint64_t &size) {
//        return size * 50;
//    }

    //TODO: model backward pass
//    static simtime_picosec backward_pass_time(const uint64_t &size) {
//        return size * 50;
//    }
//    simcpp20::event<SIM_UNIT> execute_job(simcpp20::simulation<SIM_UNIT> &, std::shared_ptr<Job>, unsigned, CollectiveScheduler *);
//
//    simcpp20::event<SIM_UNIT> allreduce(simcpp20::simulation<SIM_UNIT> &, uint64_t, std::string, std::shared_ptr<Job>);


private:
    static unsigned get_id() {
        static unsigned ID = 0;
        return ID++;
    }

    unsigned _id;
    unsigned _gpu{4};
    unsigned _gpu_capacity{4};
    std::vector<std::shared_ptr<Job>> jobs{};
    Cluster* cluster;
    std::shared_ptr<Switch> _tor;
//    std::unordered_map<unsigned, std::unordered_map<unsigned, std::shared_ptr<resource < SIM_UNIT>>>> allreduce_lock;

};


#endif //CLOUDSIMCPP_WORKER_H

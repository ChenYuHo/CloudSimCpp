#ifndef CLOUDSIMCPP_SWITCH_H
#define CLOUDSIMCPP_SWITCH_H

#include "common.h"
#include "counter.hpp"
#include "eventlist.h"
#include "network.h"
#include <tuple>
#include <set>
#include <string>
#include <unordered_map>

class Worker;

class Cluster;

class SwitchMLPacket;

class Switch : public EventSource, public PacketSink {
private:
    static unsigned get_id() {
        static unsigned ID = 0;
        return ID++;
    }

public:
    void multicast_downward(SwitchMLPacket *);
    unsigned layer = 0;
    unsigned id;
    std::vector<Worker*> machines;
    Cluster *cluster;
    Switch *upper_level_switch;
    std::vector<Switch*> lower_level_switches;
    std::unordered_map<unsigned, bool> clean_upper_level_switch_for_job{};
//    std::deque<pkt> buffer;
//    std::unordered_map<unsigned, unsigned> pkt_counter;
//    std::unordered_map<std::string, std::shared_ptr<counter<SIM_UNIT>>> counter_map;
//    std::unordered_map<unsigned, std::shared_ptr<resource<SIM_UNIT>>> pkt_lock_map;
//    std::unordered_map<unsigned, std::shared_ptr<resource<SIM_UNIT>>> pkt_lock_map2;
//    std::shared_ptr<resource<SIM_UNIT>> pkt_lock;


    std::unordered_map<unsigned, std::set<unsigned>> downward_ids_for_job{}; // JID, IDs (worker or switch) to send downward
    std::unordered_map<unsigned, unsigned> num_updates_for_job{}; // JID, num_updates
    std::unordered_map<unsigned, bool> top_level_for_job{}; // JID, top_level
    // set when placement is determined, via topology, erase when job is done

    std::unordered_map<unsigned, std::unordered_map<std::string, unsigned>> count_for_tensor_key{}; // jid, hash
    // p.ver, p.slot_idx, jid, tid, cid, iter
    std::unordered_map<unsigned, std::unordered_map<std::string, std::set<unsigned>>> seen_for_tensor_key{};

    explicit Switch(EventList &ev, Cluster *cluster, Switch *upper_level_switch) :
            EventSource(ev, "Switch"),
            cluster(cluster),
            upper_level_switch(upper_level_switch),
            id(get_id()) {
//        myprintf("Switch %d constructor invoked\n", id);
    }

    explicit Switch(unsigned id, EventList &ev, Cluster *cluster, Switch *upper_level_switch) :
            EventSource(ev, "Switch"),
            cluster(cluster),
            upper_level_switch(upper_level_switch),
            id(id) {
//        myprintf("Switch %d constructor invoked\n", id);
    }

    void doNextEvent() override;

    void receivePacket(Packet &) override;

//    unordered_map<unsigned, unsigned> pkt_counter{};
    const string &nodename() override {
        static string example = "switch";
        return example;
    };
};

#endif //CLOUDSIMCPP_SWITCH_H

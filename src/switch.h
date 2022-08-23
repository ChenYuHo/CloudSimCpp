#ifndef CLOUDSIMCPP_SWITCH_H
#define CLOUDSIMCPP_SWITCH_H

#include "common.h"
#include "counter.hpp"
#include "eventlist.h"
#include "network.h"
#include <tuple>
#include <unordered_set>
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

    std::string _nodename;

    std::unordered_map<unsigned, Route *> down_routes{};
    Route *up_route;

public:
    void multicast_downward(SwitchMLPacket *);

    unsigned layer = 0;
    unsigned id;
    std::vector<Worker *> machines{};
    Cluster *cluster;
    Switch *upper_level_switch;
    std::vector<Switch *> lower_level_switches;
    std::unordered_map<unsigned, bool> clean_upper_level_switch_for_job{};
    std::unordered_map<unsigned, std::unordered_set<unsigned>> downward_ids_for_job{}; // JID, IDs (worker or switch) to send downward
    std::unordered_map<unsigned, unsigned> num_updates_for_job{}; // JID, num_updates
    std::unordered_map<unsigned, bool> top_level_for_job{}; // JID, top_level
    // set when placement is determined, via topology, erase when job is done

    std::unordered_map<unsigned, std::unordered_map<std::string, unsigned>> count_for_tensor_key{}; // jid, hash
    // p.ver, p.slot_idx, jid, tid, cid, iter
    std::unordered_map<unsigned, std::unordered_map<std::string, std::unordered_set<unsigned>>> seen_for_tensor_key{};

    explicit Switch(EventList &ev, Cluster *cluster, Switch *upper_level_switch, Route *up_route = nullptr) :
            EventSource(ev, "Switch"),
            cluster(cluster),
            upper_level_switch(upper_level_switch),
            id(get_id()),
            _nodename(fmt::format("Switch{}", id)),
            up_route(up_route) {}

    explicit Switch(unsigned id, EventList &ev, Cluster *cluster, Switch *upper_level_switch, Route *up_route = nullptr)
            :
            EventSource(ev, "Switch"),
            cluster(cluster),
            upper_level_switch(upper_level_switch),
            id(id),
            _nodename(fmt::format("Switch{}", id)),
            up_route(up_route) {}

    void doNextEvent() override;

    void receivePacket(Packet &) override { LOG(FATAL) << "not receiving SwitchMLPacket"; };

    void receivePacket(SwitchMLPacket &pkt) override;

    SwitchMLPacket* copy_pkt(SwitchMLPacket* p, Route* route, bool upward) const;

    const string &nodename() override {
        return _nodename;
    };

    ~Switch() override {
        myprintf(12, "Switch %s destructor\n", _nodename.c_str());
        for (auto &p: down_routes) { // delete routes
            delete p.second;
        }
        down_routes.clear();
        delete up_route;
    }
};

#endif //CLOUDSIMCPP_SWITCH_H

//
// Created by Chen-Yu Ho on 9/27/21.
//

#ifndef CLOUDSIMCPP_SWITCH_H
#define CLOUDSIMCPP_SWITCH_H

#include <memory>
#include <utility>
#include <vector>
#include <deque>
#include <unordered_map>
#include "eventlist.h"
#include "network.h"
#include "packet.h"

class Worker;
class Cluster;

//class pkt;
class Switch : public EventSource, public PacketSink {
public:
    explicit Switch(EventList& ev, Cluster* cluster) :
            EventSource(ev, "Switch"),
            cluster(cluster),
            _id{get_id()}
            {};

    void doNextEvent() override;

    void receivePacket(Packet &) override;

    const string & nodename() override {
        static string example = "Hello";
        return example;
    };

    Cluster* cluster;

    unsigned id() const{return _id;};


private:
    static unsigned get_id() {
        static unsigned ID = 0;
        return ID++;
    }
    unsigned _id;
    std::vector<std::shared_ptr<Worker>> workers;
//    std::deque<pkt> buffer;
//    std::unordered_map<unsigned, unsigned> pkt_counter;
//    std::unordered_map<std::string, std::shared_ptr<counter<>>> counter_map;
//    std::unordered_map<unsigned, std::shared_ptr<resource<SIM_UNIT>>> pkt_lock_map;
//    std::unordered_map<unsigned, std::shared_ptr<resource<SIM_UNIT>>> pkt_lock_map2;
    void receivePacket(SwitchMLPacket &pkt);
    unordered_map<unsigned, unsigned> counter{};
};


#endif //CLOUDSIMCPP_SWITCH_H

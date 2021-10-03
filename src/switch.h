#ifndef CLOUDSIMCPP_SWITCH_H
#define CLOUDSIMCPP_SWITCH_H

#include "common.h"
#include "counter.hpp"
class Worker;

class Switch {
private:
    static unsigned get_id() {
        static unsigned ID = 0;
        return ID++;
    }

public:
    unsigned id;
    std::vector<std::shared_ptr<Worker>> machines;
    std::deque<pkt> buffer;
    std::unordered_map<unsigned, unsigned> pkt_counter;
    std::unordered_map<std::string, std::shared_ptr<counter<SIM_UNIT>>> counter_map;
    std::unordered_map<unsigned, std::shared_ptr<resource<SIM_UNIT>>> pkt_lock_map;
    std::unordered_map<unsigned, std::shared_ptr<resource<SIM_UNIT>>> pkt_lock_map2;
//    std::shared_ptr<resource<SIM_UNIT>> pkt_lock;

    explicit Switch() : id(get_id()) {
        printf("Switch %d constructor invoked\n", id);
    }

//    int running_jobs() {
//        int n = 0;
//        for (const std::shared_ptr<Worker>& m: machines)
//            n += m->running_jobs();
//        return n;
//    }
    simcpp20::event<SIM_UNIT> send_receive(simcpp20::simulation<SIM_UNIT> &, uint64_t, std::string, unsigned, unsigned);
};



////
//// Created by Chen-Yu Ho on 9/27/21.
////
//
//#include "switch.h"
//#include "packet.h"
//#include "cluster.h"
//#include "topology/hierarchical_topology.h"
//
//void Switch::receivePacket(Packet &pkt) {
//    auto *p = (SwitchMLPacket*)&pkt;
//    printf("[%llu] switch got packet wid %d jid %d\n", p->ts(), p->wid, p->job_id);
//    if (counter.find(p->job_id) == counter.end())
//        counter[p->job_id] = 0;
//    counter[p->job_id] += 1;
//
//    if (counter[p->job_id] == 2) {
//        printf("got all\n");
//        auto topo = (HierarchicalTopology *) cluster->topology();
//        for(unsigned i=0;i<2;i++) {
//            auto route = topo->get_tor_to_worker_path(id(), i);
//            SwitchMLPacket *multicast_p = SwitchMLPacket::newpkt(*route);
//            multicast_p->job_id = p->job_id;
//            multicast_p->wid = i;
//            multicast_p->set_ts(eventlist().now());
//            printf("[%llu] switch multicast packet wid %d jid %d\n", p->ts(), p->wid, p->job_id);
//            multicast_p->sendOn();
//        }
//        p->free();
////        p->set_ts(eventlist().now());
//    }
//
//
////    print_route(*(p->route()));
//
////    p->set_route(*(p->reverse_route()));
////    print_route(*(p->route()));
////    p->sendOn();
//}
//
//void Switch::doNextEvent() {
//
//}
//
////void Switch::receivePacket(Packet &) {
////    assert(0);
////}




////
//// Created by Chen-Yu Ho on 9/27/21.
////
//
//#ifndef CLOUDSIMCPP_SWITCH_H
//#define CLOUDSIMCPP_SWITCH_H
//
//#include <memory>
//#include <utility>
//#include <vector>
//#include <deque>
//#include <unordered_map>
//#include "eventlist.h"
//#include "network.h"
//#include "packet.h"
//
//class Worker;
//class Cluster;
//
////class pkt;
//class Switch : public EventSource, public PacketSink {
//public:
//    explicit Switch(EventList& ev, Cluster* cluster) :
//            EventSource(ev, "Switch"),
//            cluster(cluster),
//            _id{get_id()}
//            {};
//
//    void doNextEvent() override;
//
//    void receivePacket(Packet &) override;
//
//    const string & nodename() override {
//        static string example = "Hello";
//        return example;
//    };
//
//    Cluster* cluster;
//
//    unsigned id() const{return _id;};
//
//
//private:
//    static unsigned get_id() {
//        static unsigned ID = 0;
//        return ID++;
//    }
//    unsigned _id;
//    std::vector<std::shared_ptr<Worker>> workers;
////    std::deque<pkt> buffer;
////    std::unordered_map<unsigned, unsigned> pkt_counter;
////    std::unordered_map<std::string, std::shared_ptr<counter<>>> counter_map;
////    std::unordered_map<unsigned, std::shared_ptr<resource<SIM_UNIT>>> pkt_lock_map;
////    std::unordered_map<unsigned, std::shared_ptr<resource<SIM_UNIT>>> pkt_lock_map2;
//    void receivePacket(SwitchMLPacket &pkt);
//    unordered_map<unsigned, unsigned> counter{};
//};
//
//
#endif //CLOUDSIMCPP_SWITCH_H

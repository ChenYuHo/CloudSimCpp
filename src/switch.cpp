#include "switch.h"

simcpp20::event<SIM_UNIT>
Switch::send_receive(simcpp20::simulation<SIM_UNIT> &sim, uint64_t pkt_size, std::string all_ids, unsigned num_pkts_needed,
                     unsigned mid) {
    auto ptr_it = pkt_lock_map.find(mid);
    if (ptr_it == pkt_lock_map.end()) {
        pkt_lock_map.emplace(mid, std::make_shared<resource<SIM_UNIT>>(sim, 1));
    }
    auto pkt_lock = pkt_lock_map[mid];
//    if(pkt_lock==nullptr) pkt_lock = std::make_shared<resource<SIM_UNIT>>(sim, 1);
    co_await pkt_lock->request(); // one packet at a time (per link)
    auto transfer_time = pkt_transfer_time(pkt_size);
    co_await sim.timeout(transfer_time); // worker to switch
//    printf("[%llu]\tID %d Job packet arrived\n", sim.now(), all_ids);
    pkt_lock->release();
    auto it = counter_map.find(all_ids);
    if (it == counter_map.end()) {
        counter_map.emplace(all_ids, std::make_shared<counter<SIM_UNIT>>(sim, num_pkts_needed));
    }
    counter_map.at(all_ids)->add_counter();
    co_await counter_map.at(all_ids)->done(); // we have all_ids packet from all workers
    auto ptr_it2 = pkt_lock_map2.find(mid);
    if (ptr_it2 == pkt_lock_map2.end()) {
        pkt_lock_map2.emplace(mid, std::make_shared<resource<SIM_UNIT>>(sim, 1));
    }
    auto pkt_lock2 = pkt_lock_map2[mid];
    co_await pkt_lock2->request(); // one packet at a time (per link)
    counter_map.erase(all_ids);
    co_await sim.timeout(transfer_time); // ToR to worker
    pkt_lock2->release();
}


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

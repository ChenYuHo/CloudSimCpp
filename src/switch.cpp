#include "switch.h"
#include "packet.h"
#include "cluster.h"
#include "topology/hierarchical_topology.h"

//simcpp20::event<SIM_UNIT>
//Switch::send_receive(simcpp20::simulation<SIM_UNIT> &sim, uint64_t pkt_size, std::string all_ids,
//                     unsigned num_pkts_needed,
//                     unsigned mid) {
//    auto ptr_it = pkt_lock_map.find(mid);
//    if (ptr_it == pkt_lock_map.end()) {
//        pkt_lock_map.emplace(mid, std::make_shared<resource<SIM_UNIT>>(sim, 1));
//    }
//    auto pkt_lock = pkt_lock_map[mid];
////    if(pkt_lock==nullptr) pkt_lock = std::make_shared<resource<SIM_UNIT>>(sim, 1);
//    co_await pkt_lock->request(); // one packet at a time (per link)
//    auto transfer_time = pkt_transfer_time(pkt_size);
//    co_await sim.timeout(transfer_time); // worker to switch
////    myprintf("[%llu]\tID %d Job packet arrived\n", sim.now(), all_ids);
//    pkt_lock->release();
//    auto it = counter_map.find(all_ids);
//    if (it == counter_map.end()) {
//        counter_map.emplace(all_ids, std::make_shared<counter<SIM_UNIT>>(sim, num_pkts_needed));
//    }
//    counter_map.at(all_ids)->add_counter();
//    co_await counter_map.at(all_ids)->done(); // we have all_ids packet from all workers
//    auto ptr_it2 = pkt_lock_map2.find(mid);
//    if (ptr_it2 == pkt_lock_map2.end()) {
//        pkt_lock_map2.emplace(mid, std::make_shared<resource<SIM_UNIT>>(sim, 1));
//    }
//    auto pkt_lock2 = pkt_lock_map2[mid];
//    co_await pkt_lock2->request(); // one packet at a time (per link)
//    counter_map.erase(all_ids);
//    co_await sim.timeout(transfer_time); // ToR to worker
//    pkt_lock2->release();
//}

void Switch::doNextEvent() {

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
void Switch::multicast_downward(SwitchMLPacket *p) {
    for (auto dest: downward_ids_for_job[p->job_id]) {
        const Route *route = cluster->_topo->get_switch_single_hop_route(id, layer, dest, false);
        auto multicast_pkt = SwitchMLPacket::newpkt(*route);
        multicast_pkt->job_id = p->job_id;
        multicast_pkt->id = id;
        multicast_pkt->ver = p->ver;
        multicast_pkt->slot = p->slot;
        multicast_pkt->offset = p->offset;
        multicast_pkt->grad_size = p->grad_size;
        multicast_pkt->n_workers = p->n_workers;
        multicast_pkt->set_ts(eventlist().now());
        multicast_pkt->upward = false;
        multicast_pkt->tensor = p->tensor;
//        multicast_pkt->cnt = p->cnt;
//        cout<<"switch multicast:"<<p->cnt<<endl;
        multicast_pkt->sendOn();
    }
}


void Switch::receivePacket(Packet &pkt) {
    auto *p = (SwitchMLPacket *) &pkt;
    auto key = hash(p->job_id, p->tensor->tensor_id, p->tensor->iter, p->ver, p->slot);
    auto key_of_the_other_slot = hash(p->job_id, p->tensor->tensor_id, p->tensor->iter, 1 - p->ver, p->slot);
//    myprintf("%llu %d %d %d -> %d\n", p->tensor->tensor_id, p->tensor->iter, p->ver, p->slot, key);
    //    auto key = p->ver + 10 * p->slot + p->id * 1000000;
//    auto key_of_the_other_slot = (1 - p->ver) + 10 * p->slot + p->id * 1000000;
//    myprintf("[%llu] Switch %d got packet id %d ver %d slot %d off %d upward %d tid %llu, iter %llu\n", eventlist().now(),
//           id, p->id, p->ver, p->slot, p->offset, p->upward, p->tensor->tensor_id, p->tensor->tensor_id);
//    myprintf("KEY %d\n", key);

    if (!count.contains(key)) count[key] = 0;
    if (p->upward) {
        if (seen[key].contains(p->id)) {
//            myprintf("SHADOW BUFFER\n");
            // shadow buffer
        } else {
//            if (top_level_for_job[p->job_id]) {
//                myprintf("core switch got packet from switch %d for JID %d set %d slot %d\n", p->id, p->job_id, p->ver, p->slot);
//            }
//            else {
//                myprintf("ToR got packet from worker %d for JID %d set %d slot %d\n", p->id, p->job_id, p->ver, p->slot);
//            }
            seen[key].insert(p->id);
            seen[key_of_the_other_slot].erase(p->id);
//            auto &map = count[p->ver];
//            myprintf("CNT %d\n", cnt);
            count[key] =
                    ((count[key] + 1) % p->n_workers) %
                    num_updates_for_job[p->job_id];
//            myprintf("CNT666 %d\n", count[hash(p->tensor->tensor_id, p->tensor->iter)][p->slot][p->ver]);

//            if (map.find(p->slot) == map.end())
//                map[p->slot] = 0;
//            myprintf("ToR %d, jid %d, num_workers %d, num_updates %d\n", id, p->job_id, p->n_workers, num_updates_for_job[p->job_id]);
//            map[p->slot] = ((map[p->slot] + 1) % p->n_workers) % num_updates_for_job[p->job_id];
//            if self.count[pkt.ver, pkt.slot] == 1:
//            self.slots[pkt.ver, pkt.slot] = pkt.vector.copy()
//            else:
//            self.slots[pkt.ver, pkt.slot] += pkt.vector
//            myprintf("switch %d got %d/%d updates\n", id,
//                   count[key] == 0
//                   ? num_updates_for_job[p->job_id] : count[key],
//                   num_updates_for_job[p->job_id]);
            if (count[key] == 0) {
                // done aggregation
                if (top_level_for_job[p->job_id]) {
                    count[key] = p->n_workers;
//                    myprintf("core switch done aggregation, multicast from switch %d\n", id);
                    // multicast
                    multicast_downward(p);
                } else {
//                    myprintf("ToR done aggregation, sending to upper level from switch %d\n", id);
                    // when going upward, dest is determined by the topology. put 0 as a placeholder.
                    const Route *route = cluster->_topo->get_switch_single_hop_route(id, 0, 0, true);
                    // send to upper level
                    auto unicast_pkt = SwitchMLPacket::newpkt(*route);
                    unicast_pkt->job_id = p->job_id;
                    unicast_pkt->offset = p->offset;
                    unicast_pkt->id = id; // set to ToR id
                    unicast_pkt->n_workers = p->n_workers;
                    unicast_pkt->tensor = p->tensor;
                    unicast_pkt->set_ts(eventlist().now());
                    unicast_pkt->upward = true;
                    unicast_pkt->ver = p->ver;
                    unicast_pkt->slot = p->slot;
                    unicast_pkt->grad_size = p->grad_size;
                    unicast_pkt->set_ts(eventlist().now());
//                    unicast_pkt->print_info(1, eventlist().now(), 1);
//                    print_route(*route);
//                    cout<<"switch unicast:"<<p->cnt<<endl;
//                    unicast_pkt->cnt = p->cnt;
                    unicast_pkt->sendOn();

                }
            }
        }
    } else {
        // received from upper level switch
        count[key] = p->n_workers;
//        myprintf("received from upper level switch, multicast from switch %d\n", id);
        // multicast
        multicast_downward(p);
    }
    p->free();

//    if (pkt_counter.find(p->job_id) == pkt_counter.end())
//        pkt_counter[p->job_id] = 0;
//    pkt_counter[p->job_id] += 1;
//
//    if (pkt_counter[p->job_id] == p->n_updates) {
//        myprintf("got all\n");
//        auto topo = (HierarchicalTopology *) cluster->_topo;
//        for (unsigned i = 0; i < p->n_updates; i++) {
//            auto route = topo->get_tor_to_worker_path(id, i);
//            SwitchMLPacket *multicast_p = SwitchMLPacket::newpkt(*route);
//            multicast_p->job_id = p->job_id;
//            multicast_p->wid = i;
//            multicast_p->set_ts(eventlist().now());
//            myprintf("[%llu] switch multicast packet wid %d jid %d\n", eventlist().now(), p->wid, p->job_id);
//            multicast_p->sendOn();
//        }
//        p->free();
////        p->set_ts(eventlist().now());
//    }


//    print_route(*(p->route()));

//    p->set_route(*(p->reverse_route()));
//    print_route(*(p->route()));
//    p->sendOn();
}
//
//void Switch::doNextEvent() {
//
//}
//
////void Switch::receivePacket(Packet &) {
////    assert(0);
////}

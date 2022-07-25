#include "switch.h"
#include "packet.h"
#include "cluster.h"

void Switch::doNextEvent() {}

void Switch::multicast_downward(SwitchMLPacket *p) {
    for (auto dest: downward_ids_for_job[p->job_id]) {
        Route* route;
        if (down_routes.contains(dest)) [[likely]] {
            route = down_routes[dest];
        } else {
            route = cluster->_topo->get_switch_single_hop_route(id, layer, dest, false);
            down_routes[dest] = route;
        }
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
        multicast_pkt->sendOnSimple();
    }
}


void Switch::receivePacket(Packet &pkt) {
    auto *p = (SwitchMLPacket *) &pkt;
    auto key = fmt::format("{}i,s{}c{}", p->tensor->iter, p->slot, p->tensor->chunk_id);
    auto key_of_the_other_slot = fmt::format("{}v{}", key, 1 - p->ver);
    key += fmt::format("v{}", p->ver);
    myprintf(8,
             "[%llu] Switch layer %d id %d got packet from id %d ver %d slot %d off %d upward %d tid %llu, iter %llu JID %u NW %u\n",
             eventlist().now(), layer, id, p->id, p->ver, p->slot, p->offset, p->upward,
             p->tensor->tensor_id, p->tensor->iter, p->job_id, p->n_workers);
    if (p->upward) {
        auto &seen = seen_for_tensor_key[p->tensor->key];
        auto &count = count_for_tensor_key[p->tensor->key];
        if (seen[key].contains(p->id)) [[unlikely]] {
            myprintf(8, "SHADOW BUFFER\n");
            assert(false);
            // shadow buffer
        } else {
            if (top_level_for_job[p->job_id]) {
                myprintf(8, "core switch got packet from switch %d for JID %d set %d slot %d offset %u\n",
                         p->id, p->job_id, p->ver, p->slot, p->offset);
            } else {
                myprintf(8, "ToR got packet from worker %d for JID %d set %d slot %d offset %u\n",
                         p->id, p->job_id, p->ver, p->slot, p->offset);
            }
            seen[key].insert(p->id);
            seen[key_of_the_other_slot].erase(p->id);
            auto tmp_cnt = count[key];
            count[key] = ((count[key] + 1) % p->n_workers) % num_updates_for_job[p->job_id];
            myprintf(8, "Switch layer %d id %d, jid %d, num_workers %d, count %d -> %d / %d\n", layer,
                     id, p->job_id, p->n_workers, tmp_cnt, count[key], num_updates_for_job[p->job_id]);
            if (count[key] == 0) {
                // done aggregation
                if (top_level_for_job[p->job_id]) { // downward
                    count[key] = p->n_workers;
                    myprintf(8, "core switch done aggregation, multicast from switch %d\n", id);
                    // multicast
                    multicast_downward(p);
                } else {  // upward
                    myprintf(8, "ToR done aggregation, sending to upper level from switch %d\n", id);
                    // when going upward, dest is determined by the topology. put 0 as a placeholder.
//                    auto route = cluster->_topo->get_switch_single_hop_route(id, 0, 0, true);

                    // send to upper level
                    auto unicast_pkt = SwitchMLPacket::newpkt(*up_route);
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
//                    unicast_pkt->print_info(1, eventlist().now(), 1);
//                    print_route(*route);
//                    cout<<"switch unicast:"<<p->cnt<<endl;
//                    unicast_pkt->cnt = p->cnt;
                    unicast_pkt->sendOnSimple();
                }
            }
        }
    } else {
        auto &count = count_for_tensor_key[p->tensor->key];
        // received from upper level switch
        count[key] = p->n_workers;
        myprintf(8, "received from upper level switch, multicast from switch %d\n", id);
        // multicast
        multicast_downward(p);
    }
    p->free();
}

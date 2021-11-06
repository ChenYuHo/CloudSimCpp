#include "switch.h"
#include "packet.h"
#include "cluster.h"
#include "topology/hierarchical_topology.h"

void Switch::doNextEvent() {

}

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
        multicast_pkt->sendOn();
    }
}


void Switch::receivePacket(Packet &pkt) {
    auto *p = (SwitchMLPacket *) &pkt;
    auto key = hash(p->job_id, p->tensor->tensor_id, p->tensor->iter, p->ver, p->slot);
    auto key_of_the_other_slot = hash(p->job_id, p->tensor->tensor_id, p->tensor->iter, 1 - p->ver, p->slot);
    myprintf("%llu %d %d %d -> %d\n", p->tensor->tensor_id, p->tensor->iter, p->ver, p->slot, key);
    //    auto key = p->ver + 10 * p->slot + p->id * 1000000;
//    auto key_of_the_other_slot = (1 - p->ver) + 10 * p->slot + p->id * 1000000;
    myprintf("[%llu] Switch %d got packet id %d ver %d slot %d off %d upward %d tid %llu, iter %llu\n", eventlist().now(),
           id, p->id, p->ver, p->slot, p->offset, p->upward, p->tensor->tensor_id, p->tensor->tensor_id);
//    myprintf("KEY %d\n", key);

    if (!count.contains(key)) count[key] = 0;
    if (p->upward) {
        if (seen[key].contains(p->id)) {
            myprintf("SHADOW BUFFER\n");
            // shadow buffer
        } else {
            if (top_level_for_job[p->job_id]) {
                myprintf("core switch got packet from switch %d for JID %d set %d slot %d\n", p->id, p->job_id, p->ver, p->slot);
            }
            else {
                myprintf("ToR got packet from worker %d for JID %d set %d slot %d\n", p->id, p->job_id, p->ver, p->slot);
            }
            seen[key].insert(p->id);
            seen[key_of_the_other_slot].erase(p->id);
//            auto &map = count[p->ver];
//            myprintf("CNT %d\n", cnt);
            count[key] =
                    ((count[key] + 1) % p->n_workers) %
                    num_updates_for_job[p->job_id];
            myprintf("ToR %d, jid %d, num_workers %d, num_updates %d\n", id, p->job_id, p->n_workers, num_updates_for_job[p->job_id]);
//            map[p->slot] = ((map[p->slot] + 1) % p->n_workers) % num_updates_for_job[p->job_id];
//            if self.count[pkt.ver, pkt.slot] == 1:
//            self.slots[pkt.ver, pkt.slot] = pkt.vector.copy()
//            else:
//            self.slots[pkt.ver, pkt.slot] += pkt.vector
            myprintf("switch %d got %d/%d updates\n", id,
                   count[key] == 0
                   ? num_updates_for_job[p->job_id] : count[key],
                   num_updates_for_job[p->job_id]);
            if (count[key] == 0) {
                // done aggregation
                if (top_level_for_job[p->job_id]) {
                    count[key] = p->n_workers;
                    myprintf("core switch done aggregation, multicast from switch %d\n", id);
                    // multicast
                    multicast_downward(p);
                } else {
                    myprintf("ToR done aggregation, sending to upper level from switch %d\n", id);
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
        myprintf("received from upper level switch, multicast from switch %d\n", id);
        // multicast
        multicast_downward(p);
    }
    p->free();
}

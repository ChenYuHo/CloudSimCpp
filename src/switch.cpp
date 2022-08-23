#include "switch.h"
#include "packet.h"
#include "cluster.h"

void Switch::doNextEvent() {}

void Switch::multicast_downward(SwitchMLPacket *p) {
    for (auto dest: downward_ids_for_job[p->job_id]) {
        Route *route;
        if (down_routes.contains(dest)) [[likely]] {
            route = down_routes[dest];
        } else {
            route = cluster->_topo->get_switch_single_hop_route(id, layer, dest, false);
            down_routes[dest] = route;
        }
        auto multicast_pkt = copy_pkt(p, route, false);
        if (!machines.empty()) { // sending to workers
            auto worker = cluster->worker_map[dest];
            multicast_pkt->originated_worker = worker;
            multicast_pkt->route_to_tor = worker->route_to_tor;
            multicast_pkt->tensor = worker->tensors_for_job[p->job_id][p->tensor->tensor_id];
        }

        myprintf(8, "[%lu] Switch layer %d id %d multicast packet downward to wid/sid %d %s\n",
                 eventlist().now(), layer, id, dest, multicast_pkt->to_str().c_str());
        multicast_pkt->sendOnSimple();
    }
}


void Switch::receivePacket(SwitchMLPacket &pkt) {
    auto p = &pkt;
    auto key = fmt::format("s{}v{}", p->slot, p->ver);
    auto key_of_the_other_slot = fmt::format("s{}v{}", p->slot, 1 - p->ver);
    myprintf(8, "[%lu] Switch layer %d id %d got packet %s\n",
             eventlist().now(), layer, id, p->to_str().c_str());
    if (p->upward) {
        auto &seen = seen_for_tensor_key[p->tensor->key];
        auto &count = count_for_tensor_key[p->tensor->key];
        if (seen[key].contains(p->id)) [[unlikely]] { // shadow buffer
            if (count[key] == p->n_workers) {
                auto dest = p->id;
                assert(down_routes.contains(dest));
                Route *route = down_routes[dest];
                auto unicast_pkt = copy_pkt(p, route, false);
                myprintf(11, "[%lu] Switch layer %d id %d reply shadow buffer packet downward to wid/sid %d %s\n",
                         eventlist().now(), layer, id, dest, unicast_pkt->to_str().c_str());
                unicast_pkt->sendOnSimple();
            } else if (top_level_for_job[p->job_id] && count[key] == 0) {
                auto unicast_pkt = copy_pkt(p, up_route, true);
                myprintf(11, "[%lu] Switch layer %d id %d forward shadow buffer request packet upward %s\n",
                         eventlist().now(), layer, id, unicast_pkt->to_str().c_str());
                unicast_pkt->sendOnSimple();
            } // else drop (free) packet
        } else {
            seen[key].insert(p->id);
            seen[key_of_the_other_slot].erase(p->id);
            count[key] = ((count[key] + 1) % p->n_workers) % num_updates_for_job[p->job_id];
            myprintf(8, "[%lu] Switch layer %d id %d, jid %d, num_workers %d, count %d / %d\n", eventlist().now(),
                     layer, id, p->job_id, p->n_workers, count[key] + 1, num_updates_for_job[p->job_id]);
            if (count[key] == 0) {
                // done aggregation
                if (top_level_for_job[p->job_id]) { // downward
                    count[key] = p->n_workers;
                    multicast_downward(p);
                } else {  // upward
                    // send to upper level
                    auto unicast_pkt = copy_pkt(p, up_route, true);
                    myprintf(8, "[%lu] Switch layer %d id %d send packet upward %s\n",
                             eventlist().now(), layer, id, unicast_pkt->to_str().c_str());
                    unicast_pkt->sendOnSimple();
                }
            } // else drop (free) packet
        }
    } else {
        auto &count = count_for_tensor_key[p->tensor->key];
        // received from upper level switch
        count[key] = p->n_workers;
        multicast_downward(p);
    }
    p->free();
}

SwitchMLPacket *Switch::copy_pkt(SwitchMLPacket *p, Route *route, bool upward) const {
    auto copy = SwitchMLPacket::newpkt(*route);
    copy->job_id = p->job_id;
    copy->id = id;
    copy->ver = p->ver;
    copy->slot = p->slot;
    copy->offset = p->offset;
    copy->grad_size = p->grad_size;
    copy->n_workers = p->n_workers;
    copy->set_ts(p->ts());
    copy->upward = upward;
    copy->tensor = p->tensor;
    copy->originated_worker = p->originated_worker;
    copy->route_to_tor = p->route_to_tor;
    return copy;
}

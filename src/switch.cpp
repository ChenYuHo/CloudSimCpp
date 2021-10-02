//
// Created by Chen-Yu Ho on 9/27/21.
//

#include "switch.h"
#include "packet.h"
#include "cluster.h"
#include "topology/hierarchical_topology.h"

void Switch::receivePacket(Packet &pkt) {
    auto *p = (SwitchMLPacket*)&pkt;
    printf("[%llu] switch got packet wid %d jid %d\n", p->ts(), p->wid, p->job_id);
    if (counter.find(p->job_id) == counter.end())
        counter[p->job_id] = 0;
    counter[p->job_id] += 1;

    if (counter[p->job_id] == 2) {
        printf("got all\n");
        auto topo = (HierarchicalTopology *) cluster->topology();
        for(unsigned i=0;i<2;i++) {
            auto route = topo->get_tor_to_worker_path(id(), i);
            SwitchMLPacket *multicast_p = SwitchMLPacket::newpkt(*route);
            multicast_p->job_id = p->job_id;
            multicast_p->wid = i;
            multicast_p->set_ts(eventlist().now());
            printf("[%llu] switch multicast packet wid %d jid %d\n", p->ts(), p->wid, p->job_id);
            multicast_p->sendOn();
        }
        p->free();
//        p->set_ts(eventlist().now());
    }


//    print_route(*(p->route()));

//    p->set_route(*(p->reverse_route()));
//    print_route(*(p->route()));
//    p->sendOn();
}

void Switch::doNextEvent() {

}

//void Switch::receivePacket(Packet &) {
//    assert(0);
//}

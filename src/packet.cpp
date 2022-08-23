//
// Created by Chen-Yu Ho on 9/28/21.
//

#include "packet.h"

PacketDB<SwitchMLPacket> SwitchMLPacket::_packetdb;

PacketSink *SwitchMLPacket::sendOnSimple() {
    PacketSink *nextsink = _route->at(_nexthop++);
    myprintf(8, "packet %s traveling to %s\n", this->to_str().c_str(), nextsink->nodename().c_str());
    nextsink->receivePacket(*this);
    return nextsink;
}

SwitchMLPacket::~SwitchMLPacket() {
//    delete _route;
}

SwitchMLPacket* SwitchMLPacket::construct_resend_pkt(Worker* worker, simtime_picosec ts) const {
    auto p = newpkt(*worker->route_to_tor);
    p->id = worker->id;
    p->ver = ver;
    p->slot = slot;
    p->offset = offset;
    p->upward = true;
    p->grad_size = grad_size;
    p->n_workers = n_workers;
    p->job_id = job_id;
    p->tensor = worker->tensors_for_job[job_id][tensor->tensor_id];
    p->set_ts(ts);
    p->originated_worker = worker;
    p->route_to_tor = worker->route_to_tor;
//    myprintf(8, "[%lu] mid %u send packet %s\n", ts, id, p->to_str().c_str());
//    p->sendOnSimple();
    return p;
}

#include "simplepipe.h"
#include <glog/logging.h>

void SimplePipe::doNextEvent() {
    if (_inflight.empty()) {
        LOG(INFO) << _name << " inflight is empty\n";
        return;
    }

    auto pkt = _inflight.front().second;
    _inflight.pop();
    // tell the packet to move itself on to the next hop
    pkt->sendOnSimple();

    // notify the eventlist we've another event pending
    _eventlist.sourceIsPending(*this, _inflight.front().first);
    LOG(INFO) << _name << " enqueue \n";
}

void SimplePipe::receivePacket(Packet &pkt) {
    if (_inflight.empty()) {
        /* no packets currently inflight; need to notify the eventlist
           we've an event pending */
        LOG(INFO) << _name << " pipe receive packet and enqueue " << endl;
        _eventlist.sourceIsPendingRel(*this, _delay);
    } else {
        LOG(INFO) << _name << " !_inflight.empty()\n";
    }
    _inflight.push(std::make_pair(_eventlist.now() + _delay, &pkt));
}

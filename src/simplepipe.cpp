#include "simplepipe.h"

void SimplePipe::receivePacket(Packet &pkt) {
    if (_inflight.empty()) {
        /* no packets currently inflight; need to notify the eventlist
           we've an event pending */
        _eventlist.sourceIsPendingRel(*this, _delay);
    }
    _inflight.push(std::make_pair(_eventlist.now() + _delay, &pkt));
}

void SimplePipe::doNextEvent() {
    if (_inflight.empty()) return;

    auto pkt = _inflight.front().second;
    _inflight.pop();
    // tell the packet to move itself on to the next hop
    pkt->sendOn();

    // notify the eventlist we've another event pending
    _eventlist.sourceIsPending(*this, _inflight.front().first);
}

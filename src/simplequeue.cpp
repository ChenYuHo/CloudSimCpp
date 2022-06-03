#include "simplequeue.h"
#include <glog/logging.h>

void SimpleQueue::doNextEvent() {
    // dequeue the packet
    LOG_IF(FATAL, _enqueued.empty()) << "queue can't be empty";
    Packet *pkt = _enqueued.front();
    _enqueued.pop();
    _queuesize -= pkt->size();
    // move on to the next pipe
    pkt->sendOn();
    if (!_enqueued.empty()) {
        _eventlist.sourceIsPendingRel(*this, drainTime(_enqueued.front()));
    }
}

void SimpleQueue::receivePacket(Packet &pkt) {
    if (_queuesize + pkt.size() > _maxsize) {
        // drop the packet
        pkt.free();
        LOG(WARNING) << "threshold exceeded";
        return;
    }

    // enqueue the packet
    _enqueued.push(&pkt);
    _queuesize += pkt.size();
    if (_enqueued.size() == 1) {
        // schedule the dequeue event
        _eventlist.sourceIsPendingRel(*this, drainTime(_enqueued.front()));
    }
}

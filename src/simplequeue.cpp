#include "simplequeue.h"
#include <glog/logging.h>

void SimpleQueue::doNextEvent() {
    // dequeue the packet
    Packet *pkt = _enqueued.front();
    _enqueued.pop();
    _queuesize -= pkt->size();
    // move on to the next pipe
    pkt->sendOnSimple();
    if (!_enqueued.empty()) {
//        LOG(INFO) << _name << " doNextEvent queue " << drainTime(_enqueued.front());
        _eventlist.sourceIsPendingRel(*this, drainTime(_enqueued.front()));
//    } else {
//        LOG(INFO) << _name << " _enqueued.empty()\n";
    }
}

// receiveMultiplePacket
void SimpleQueue::receivePacket(Packet &pkt) {
    if (_queuesize + pkt.size() > _maxsize) {
        // drop the tail packet
        pkt.free();
        LOG(WARNING) << _name << "packet dropped due to buffer overflow";
        return;
    }

    // enqueue the packet
    _enqueued.push(&pkt);
    _queuesize += pkt.size();
    if (_enqueued.size() == 1) {
//        LOG(INFO) << _name << " receive packet and enqueue " << drainTime(_enqueued.front());
        // schedule the dequeue event
        _eventlist.sourceIsPendingRel(*this, drainTime(_enqueued.front()));
//    } else {
//        LOG(INFO) << _name << " _enqueued.size() != 1\n";
    }
}

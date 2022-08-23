#include "simplequeue.h"
#include "cluster.h"
#include <time.h>
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
    LOG(FATAL) << "not receiving SwitchMLPacket";
    if (_queuesize + pkt.size() > _maxsize) {
        // drop the tail packet
        pkt.free();
        LOG(FATAL) << _name << " packet dropped due to buffer overflow";
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

void SimpleQueue::receivePacket(SwitchMLPacket &pkt) {
    if (_queuesize + pkt.size() > _maxsize) {
        // overflow, drop the tail packet
        // ((unsigned) time(nullptr) % 10000) && drop
        //  drop--;
        num_pkt_drops++;
        if (to_worker) { // only resend from originated_wid
            auto p = pkt.construct_resend_pkt(pkt.originated_worker, eventlist().now());
            auto diff_to_start_time = eventlist().now() - pkt.ts();
            auto send_in_time = diff_to_start_time > timeFromMs(10) ? 1 : timeFromMs(10) - diff_to_start_time;
            auto &_sim = eventlist().sim();
            auto ev = _sim.event();
            ev.add_callback([p](const auto &) { p->sendOnSimple(); });
            _sim.schedule(ev, send_in_time);
            myprintf(10, "[%lu] packet dropped from %s, resending pkt %s at %lu\n",
                     eventlist().now(), _name.c_str(), p->to_str().c_str(), eventlist().now() + send_in_time);
        } else { // resend from all workers
            for (auto wid: pkt.tensor->job->wids_allocated) {
                auto worker = pkt.tensor->machine->cluster->worker_map[wid];
                auto p = pkt.construct_resend_pkt(worker, eventlist().now());
                auto elapsed = eventlist().now() - pkt.ts();
                auto schedule_delay = elapsed > timeFromMs(10)
                                      ? 10//RNG::gen_rand_1us()
                                      : timeFromMs(10) - elapsed;// + 100 * RNG::gen_rand_1us();
                auto time_to_send = eventlist().now() + schedule_delay;
                p->set_ts(time_to_send);
                auto &_sim = eventlist().sim();
                auto ev = _sim.event();
                ev.add_callback([p](const auto &) { p->sendOnSimple(); });
                _sim.schedule(ev, schedule_delay);
                myprintf(10, "[%lu] packet dropped from %s, resending pkt %s at %lu\n",
                         eventlist().now(), _name.c_str(), p->to_str().c_str(), time_to_send);
            }
        }
        pkt.free();
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

SimpleQueue::~SimpleQueue() {
    LOG_IF(INFO, num_pkt_drops > 0) << _name << " has lost " << num_pkt_drops << " pkts";
}

#ifndef CLOUDSIMCPP_SIMPLESimpleQueue_H
#define CLOUDSIMCPP_SIMPLESimpleQueue_H

#include "config.h"
#include "eventlist.h"
#include "network.h"
#include "common.h"
#include "packet.h"
#include "job.h"
#include "worker.h"
#include <queue>

class SimpleQueue : public EventSource, public PacketSink {
public:
    SimpleQueue(linkspeed_bps bitrate, mem_b maxsize, EventList &eventlist, bool to_worker=false) :
            EventSource(eventlist, "simplequeue"),
            _maxsize(maxsize), _bitrate(bitrate),
            _ps_per_byte(8000000000000ULL / _bitrate), // 8e12
            to_worker(to_worker),
            _queuesize(0) {};

    void receivePacket(Packet &pkt) override;

    void receivePacket(SwitchMLPacket &pkt) override;

    void doNextEvent() override;

    inline simtime_picosec drainTime(Packet *pkt) const {
        return (simtime_picosec) (pkt->size() * _ps_per_byte);
    }

    const string &nodename() override { return _nodename; };

    bool to_worker{false};

    uint64_t num_pkt_drops{0};

//    int drop{10};

    ~SimpleQueue() override;

protected:
    linkspeed_bps _bitrate;
    simtime_picosec _ps_per_byte;
    mem_b _queuesize;
    std::queue<Packet *> _enqueued;
    mem_b _maxsize;
    std::string _nodename{"simplequeue"};
};

#endif //CLOUDSIMCPP_SIMPLESimpleQueue_H

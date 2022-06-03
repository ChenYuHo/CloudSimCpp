#ifndef CLOUDSIMCPP_SIMPLESimpleQueue_H
#define CLOUDSIMCPP_SIMPLESimpleQueue_H

#include "config.h"
#include "eventlist.h"
#include "network.h"
#include "common.h"
#include <queue>

class SimpleQueue : public EventSource, public PacketSink {
public:
    SimpleQueue(linkspeed_bps bitrate, mem_b maxsize, EventList &eventlist) :
            EventSource(eventlist, "queue"),
            _maxsize(maxsize), _bitrate(bitrate),
            _ps_per_byte(8000000000000ULL / _bitrate), // 8e12
            _queuesize(0) {};

    void receivePacket(Packet &pkt) override;

    void doNextEvent() override;

    inline simtime_picosec drainTime(Packet *pkt) const {
        return (simtime_picosec) (pkt->size() * _ps_per_byte);
    }

    const string &nodename() override { return _nodename; };

protected:
    linkspeed_bps _bitrate;
    simtime_picosec _ps_per_byte;
    mem_b _queuesize;
    std::queue<Packet *> _enqueued;
    mem_b _maxsize;
    std::string _nodename{"queue"};
};

#endif //CLOUDSIMCPP_SIMPLESimpleQueue_H

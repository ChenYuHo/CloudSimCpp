#ifndef CLOUDSIMCPP_SIMPLEPIPE_H
#define CLOUDSIMCPP_SIMPLEPIPE_H

#include <queue>
#include "config.h"
#include "eventlist.h"
#include "network.h"

class SimplePipe : public EventSource, public PacketSink {
public:
    SimplePipe(simtime_picosec delay, EventList &eventlist) : EventSource(eventlist, "pipe"), _delay(delay) {};

    void receivePacket(Packet &pkt) override; // inherited from PacketSink
    void doNextEvent() override; // inherited from EventSource
    const string &nodename() override { return _nodename; };
private:
    simtime_picosec _delay;
    typedef pair<simtime_picosec, Packet *> pktrecord_t;
    std::queue<pktrecord_t> _inflight; // the packets in flight (or being serialized)
    std::string _nodename{"pipe"};
};

#endif //CLOUDSIMCPP_SIMPLEPIPE_H

//
// Created by Chen-Yu Ho on 9/28/21.
//

#ifndef CLOUDSIMCPP_PACKET_H
#define CLOUDSIMCPP_PACKET_H
#include "network.h"

class SwitchMLPacket : public Packet {
public:
    inline static SwitchMLPacket* newpkt(const Route &route) {
        SwitchMLPacket* p = _packetdb.allocPacket();
        p->set_route(route);
        p->_type = IP;
        return p;
    }

    void free() {_packetdb.freePacket(this);}
    virtual ~SwitchMLPacket(){}
    inline simtime_picosec ts() const {return _ts;}
    inline void set_ts(simtime_picosec ts) {_ts = ts;}
    unsigned job_id{};
    unsigned slot{};
    unsigned wid{};
protected:
    simtime_picosec _ts;
    static PacketDB<SwitchMLPacket> _packetdb;

};


#endif //CLOUDSIMCPP_PACKET_H

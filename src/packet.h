//
// Created by Chen-Yu Ho on 9/28/21.
//

#ifndef CLOUDSIMCPP_PACKET_H
#define CLOUDSIMCPP_PACKET_H

#include "network.h"
#include "common.h"
#include "worker.h"
#include <unordered_set>

class SwitchMLPacket : public Packet {
public:
    inline static SwitchMLPacket *newpkt(const Route &route) {
        SwitchMLPacket *p = _packetdb.allocPacket();
        p->set_size(SWITCHML_PKT_SIZE);
        p->set_route(route);
        p->_type = IP;
        p->_nexthop = 0;
        return p;
    }

    inline static SwitchMLPacket *newpkt(const Route &route, SwitchMLPacket *other) {
        SwitchMLPacket *p = _packetdb.allocPacket();
//        delete p->_route;
        p->set_route(route);
        p->_type = IP;
        p->_nexthop = 0;
        p->tensor = other->tensor;
        p->job_id = other->job_id;
        p->n_workers = other->n_workers;
        p->grad_size = other->grad_size;
        p->slot = other->slot;

        return p;
    }

    void free() override {
//        delete _route;
        _packetdb.freePacket(this);
    }

    ~SwitchMLPacket() override;

    inline simtime_picosec ts() const { return _ts; }

    inline void set_ts(simtime_picosec ts) { _ts = ts; }

    unsigned job_id{};
    unsigned n_workers{0};
    uint64_t grad_size{};
    bool top_level{false};
    bool upward{true};
    unsigned id{}; // worker or switch id
    unsigned slot{};
    unsigned ver{};
    unsigned offset{};
    Tensor *tensor{};
    std::unordered_set<unsigned> wids{};
    Worker* originated_worker;
    Route *route_to_tor;
    PacketSink* sendOnSimple() override;

    SwitchMLPacket* construct_resend_pkt(Worker* worker, simtime_picosec ts) const;

    inline std::string to_str() {
        return fmt::format("jid {} tid {} iter {} nw {} gs {} upward {} wid/sid {} slot {} ver {} offset {}",
                           job_id, tensor->tensor_id, tensor->iter, n_workers, grad_size, upward, id, slot,
                           ver, offset);
    }

protected:
    simtime_picosec _ts{};
    static PacketDB<SwitchMLPacket> _packetdb;

};


#endif //CLOUDSIMCPP_PACKET_H

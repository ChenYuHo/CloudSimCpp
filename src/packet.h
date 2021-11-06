//
// Created by Chen-Yu Ho on 9/28/21.
//

#ifndef CLOUDSIMCPP_PACKET_H
#define CLOUDSIMCPP_PACKET_H
#include "network.h"
#include "common.h"

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
        p->n_updates = other->n_updates;
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
    unsigned n_updates{};
    unsigned n_workers{0};
    uint64_t grad_size{};
    bool top_level{false};
    bool upward{true};
    unsigned id{}; // worker or switch id
    unsigned slot{};
    unsigned ver{};
    unsigned offset{};
    shared_ptr<Tensor> tensor{};

//    void print_info(unsigned type, SIM_UNIT now, unsigned to_id) {
//        auto &from_id = id;
//        switch (type) {
//            case 0: // worker to ToR
////                myprintf("[%llu] worker %d sent packet to ToR, ver %d slot %d offset %d upward %d grad_size %llu n_workers %d job_id %d \n", now, from_id, ver, slot, offset, upward, grad_size, n_workers, job_id);
////                p->id = id;
////                p->ver = ver;
////                p->slot = slot;
////                p->offset = start;
////                p->upward = true;
////                p->grad_size = grad_size;
////                p->n_workers = tensor->job->num_workers_allocated;
////                p->job_id = tensor->job->id;
////                p->tensor = tensor;
//                break;
//            case 1: // ToR to worker
////                myprintf("");
//                break;
//            case 2: // switch to switch upward
////                myprintf("[%llu] switch %d sent packet upward, ver %d slot %d offset %d upward %d grad_size %llu n_workers %d job_id %d \n", now, from_id, ver, slot, offset, upward, grad_size, n_workers, job_id);
//
//                break;
//            case 3: // switch to switch downward
////                myprintf("");
//                break;
//            default:
//                return;
//        }
//    }

protected:
    simtime_picosec _ts;
    static PacketDB<SwitchMLPacket> _packetdb;

};


#endif //CLOUDSIMCPP_PACKET_H

#include "common.h"


simtime_picosec pkt_transfer_time(uint64_t pkt_size) {
    return pkt_size/*num elements*/* 320; // 100Gbps
}
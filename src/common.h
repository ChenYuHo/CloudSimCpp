#ifndef CLOUDSIMCPP_COMMON_H
#define CLOUDSIMCPP_COMMON_H

#include <cstdint>
#include <memory>
#include "resource.hpp"

typedef uint64_t simtime_picosec;
typedef simtime_picosec SIM_UNIT;
const static uint32_t RTT = 1;
class Worker;
class Job;

struct Tensor {
    uint64_t size;
    uint64_t allreduced_size;
    Worker *machine;
    std::shared_ptr<Job> job;
    unsigned tensor_id;
    unsigned chunk_id;
    unsigned iter;
    resource<SIM_UNIT> lock;
};

struct pkt {
    uint64_t size;
    Worker *machine;
    unsigned tid;
    unsigned cid;
    std::shared_ptr<Job> job;
};

struct config {
    int m_per_tor;
    int n_tor;
    int g_per_m;
};

simtime_picosec pkt_transfer_time(uint64_t pkt_size);

struct pair_hash {
    template<class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2> &pair) const {
        return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
    }
};

#endif //CLOUDSIMCPP_COMMON_H

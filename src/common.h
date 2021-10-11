#ifndef CLOUDSIMCPP_COMMON_H
#define CLOUDSIMCPP_COMMON_H

#include <cstdint>
#include <memory>
#include <utility>
#include <set>
#include "resource.hpp"

typedef uint64_t simtime_picosec;
typedef simtime_picosec SIM_UNIT;
const static uint32_t RTT = 1;

class Worker;

class Job;

//struct Tensor {
//    uint64_t size;
//    uint64_t allreduced_size;
//    Worker *machine;
//    std::shared_ptr<Job> job;
//    unsigned tensor_id;
//    unsigned chunk_id;
//    unsigned iter;
//    resource<SIM_UNIT> lock;
//};


//Tensor{grad_size, 0,
//this, job, tensor_id++, 0,
//0, resource(sim, 1)}

class Tensor {
private:
    static uint64_t get_id() {
        static uint64_t ID = 0;
        return ID++;
    }

public:
    uint64_t size;
    uint64_t allreduced_size{0};
    Worker *machine;
    std::shared_ptr<Job> job;
    unsigned chunk_id{0};
    unsigned iter{0};
    resource<SIM_UNIT> lock;
    uint64_t tensor_id;
    std::set<unsigned> received_pkts{};
    unsigned num_pkts_expected{0};

    Tensor(uint64_t size, Worker *machine, std::shared_ptr<Job> job,
           simcpp20::simulation<SIM_UNIT> &sim) :
            size(size), machine(machine), job(std::move(job)),
            tensor_id(get_id()), lock(resource(sim, 1)) {
//        printf("Tensor %lu constructor invoked\n", tensor_id);
    }

    Tensor(uint64_t id, uint64_t size, Worker *machine, std::shared_ptr<Job> job,
           simcpp20::simulation<SIM_UNIT> &sim) :
            size(size), machine(machine), job(std::move(job)),
            tensor_id(id), lock(resource(sim, 1)) {
//        printf("Tensor %lu constructor invoked\n", id);
    }
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

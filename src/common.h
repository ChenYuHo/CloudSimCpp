#ifndef CLOUDSIMCPP_COMMON_H
#define CLOUDSIMCPP_COMMON_H

#include <cstdint>
#include <cstdarg>
#include <memory>
#include <utility>
#include <set>
#include "resource.hpp"
#include "CppProgressBar.h"


#define SIMULATE_ALLREDUCE 1
#define DEFAULTDATASIZE 9000
#define HOST_NIC 100000 // host nic speed in Mbps
#define SWITCH_BUFFER 100000000 // in bytes, per queue (port)
#define SWITCH_PORTS 8
#define GPUS_PER_NODE 2
#define PRINT_MASK 0b00000000000000000000000000000100
// 1
// 2 worker.cpp lock for tensors
// 4 forward backward allreduce timestamp
// 8 packet tracing
typedef uint64_t simtime_picosec;
typedef simtime_picosec SIM_UNIT;
const static uint32_t RTT = 1; // us
const static int SWITCHML_PKT_SIZE = DEFAULTDATASIZE;
const static unsigned CHUNK_SIZE = 262144;
//const static unsigned NUM_SLOTS=512; // pool size
//const static unsigned NUM_UPDATES = (SWITCHML_PKT_SIZE - (8 + 14 + 20 + 8 + 6 + 4 + 12)) / 4;
const static unsigned NUM_SLOTS = 5; // pool size
const static unsigned NUM_UPDATES = 10;// (SWITCHML_PKT_SIZE - (8 + 14 + 20 + 8 + 6 + 4 + 12)) / 4;

extern CppProgressBar cpb;

int myprintf(const char *, ...);

int myprintf(uint32_t, const char *, ...);

int myprintf(const char *, va_list, int);


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
    unsigned iter{0};
    // lock is specific to tensor, however switch doesn't know how to retrive tensor
//    resource<SIM_UNIT> lock;
//    resource<SIM_UNIT> allreduce_lock;
    uint64_t tensor_id;
//    std::vector<std::set<unsigned>> received_pkts{2*NUM_SLOTS};
    std::set<unsigned> received_pkts;
    unsigned num_pkts_expected{0};
    uint64_t forward_pass_time{};
    uint64_t backward_pass_time{};
    SIM_UNIT allreduce_start{};

    Tensor(uint64_t size, Worker *machine, std::shared_ptr<Job> job,
           simcpp20::simulation<SIM_UNIT> &sim) :
            size(size), machine(machine), job(std::move(job)),
            tensor_id(get_id()) {
//        myprintf("Tensor %lu constructor invoked\n", tensor_id);
    }

    Tensor(uint64_t id, uint64_t size, Worker *machine, std::shared_ptr<Job> job,
           simcpp20::simulation<SIM_UNIT> &sim) :
            size(size), machine(machine), job(std::move(job)),
            tensor_id(id) {
//        myprintf("Tensor %lu constructor invoked\n", id);
    }

    Tensor(uint64_t id, uint64_t f, uint64_t b, uint64_t size, Worker *machine, std::shared_ptr<Job> job,
           simcpp20::simulation<SIM_UNIT> &sim) :
            size(size), forward_pass_time(f), backward_pass_time{b}, machine(machine), job(std::move(job)),
            tensor_id(id) {
//        myprintf("Tensor %lu constructor invoked\n", id);
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

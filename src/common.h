#ifndef CLOUDSIMCPP_COMMON_H
#define CLOUDSIMCPP_COMMON_H

#include <cstdint>
#include <cstdarg>
#include <memory>
#include <utility>
#include <set>
#include "resource.hpp"
#include "CppProgressBar.h"

typedef uint64_t simtime_picosec;
typedef simtime_picosec SIM_UNIT;
extern const uint32_t& DEFAULTDATASIZE;
extern const uint32_t& HOST_NIC; // host nic speed in Mbps
extern const uint32_t& SWITCH_BUFFER; // in bytes, per queue (port)
extern const uint32_t& SWITCH_PORTS;
extern const uint32_t& GPUS_PER_NODE;
extern const uint32_t& PRINT_MASK;
extern const uint32_t& RTT; // us
extern const uint32_t& SWITCHML_PKT_SIZE;
extern const uint32_t& CHUNK_SIZE;
extern const uint32_t& NUM_SLOTS; // pool size
extern const uint32_t& NUM_UPDATES;

extern CppProgressBar cpb;

int myprintf(const char *, ...);

int myprintf(uint32_t, const char *, ...);

int myprintf(const char *, va_list, int);


class Worker;

class Job;

class Tensor {
private:
    static uint64_t get_id() {
        static uint64_t ID = 0;
        return ID++;
    }

public:
    uint64_t key{};
    uint64_t size;
    uint64_t allreduced_size{0};
    Worker *machine;
    Job *job;
    unsigned iter{0};
    // lock is specific to tensor, however switch doesn't know how to retrive tensor
//    resource<SIM_UNIT> lock;
//    resource<SIM_UNIT> allreduce_lock;
    uint64_t tensor_id;
    uint64_t chunk_id{0};
    unsigned num_pkts_expected{0};
    uint64_t forward_pass_time{};
    uint64_t backward_pass_time{};
    SIM_UNIT allreduce_start{};

    Tensor(uint64_t, Worker *, Job *, simcpp20::simulation<SIM_UNIT> &);

    Tensor(uint64_t, uint64_t, Worker *, Job *, simcpp20::simulation<SIM_UNIT> &);

    Tensor(uint64_t, uint64_t, uint64_t, uint64_t, Worker *, Job *, simcpp20::simulation<SIM_UNIT> &);
};

uint64_t get_key(uint64_t job_id, uint64_t tensor_id);
uint64_t get_key(Tensor*);

std::string getenv(const std::string& variable_name, const std::string& default_value) noexcept;

#endif //CLOUDSIMCPP_COMMON_H

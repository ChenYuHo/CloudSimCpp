#include "common.h"
#include <string>
#include "CppProgressBar.h"
#include "job.h"
#include "network.h"
#include <glog/logging.h>

CppProgressBar cpb;

std::string getenv(const std::string &variable_name, const std::string &default_value) noexcept {
    const char *value = getenv(variable_name.c_str());
    return value ? value : default_value;
}

namespace {
    uint32_t DEFAULTDATASIZE_impl;
    uint32_t HOST_NIC_impl; // host nic speed in Mbps
    uint32_t SWITCH_BUFFER_impl; // in bytes, per queue (port)
    int32_t SWITCH_PORTS_impl;
    uint32_t GPUS_PER_NODE_impl;
    uint32_t RTT_impl; // us
    uint32_t CHUNK_SIZE_impl;
    uint32_t NUM_SLOTS_impl; // pool size
    uint32_t PRINT_MASK_impl;
    uint32_t NUM_UPDATES_impl;
    bool COLLECTIVE_STATISTICS_impl;

    bool read_env_and_print() noexcept {
        auto gbps = std::stoul(getenv("NIC_Gbps", "100"));
        HOST_NIC_impl = 1000 * gbps;
        SWITCH_BUFFER_impl = std::stoul(getenv("SWITCH_BUFFER_BYTES", "100000000"));
        SWITCH_PORTS_impl = std::stoi(getenv("SWITCH_PORTS", "8"));
        GPUS_PER_NODE_impl = std::stoul(getenv("GPUS_PER_NODE", "4"));
        RTT_impl = std::stoul(getenv("RTT_us", "1"));
        CHUNK_SIZE_impl = std::stoul(getenv("CHUNK_SIZE", "262144"));

        // SwitchML paper states use 128 and 512 as the pool size for 10 and 100 Gbps, respectively (and 128 otherwise)
        const char *n = getenv("NUM_SLOTS");
        if (n) NUM_SLOTS_impl = std::stoul(n);
        else NUM_SLOTS_impl = (gbps == 100) ? 512 : 128;

        PRINT_MASK_impl = std::stoul(getenv("PRINT_MASK", std::to_string(1 << 3 | 1 << 7)));

        const char *m = getenv("MTU");
        const char *u = getenv("NUM_UPDATES");
        if (m) {
            DEFAULTDATASIZE_impl = std::stoul(m);
            if (u) {
                // both m and u
                NUM_UPDATES_impl = std::stoul(u);
                if (8 + 14 + 20 + 8 + 16 + NUM_UPDATES_impl * 4 + 4 + 12 > DEFAULTDATASIZE_impl)
                    LOG(FATAL) << fmt::format("NUM_UPDATES={} exceeds MTU={} capacity", NUM_UPDATES_impl,
                                              DEFAULTDATASIZE_impl);
            } else {
                // no u but m
                NUM_UPDATES_impl = (DEFAULTDATASIZE_impl - (8 + 14 + 20 + 8 + 16 + 4 + 12)) / 4;
            }
        } else if (u) {
            // no m but u
            NUM_UPDATES_impl = std::stoul(u);
            DEFAULTDATASIZE_impl = 8 + 14 + 20 + 8 + 16 + NUM_UPDATES_impl * 4 + 4 + 12;
        } else {
            // neither
            DEFAULTDATASIZE_impl = 1500;
            NUM_UPDATES_impl = 256;
        }

        COLLECTIVE_STATISTICS_impl = strtobool(getenv("COLLECTIVE_STATISTICS", "0"));
        printf("MTU %u\nNIC_Gbps %lu\nSWITCH_BUFFER_BYTES %u\n"
               "SWITCH_PORTS %u\nGPUS_PER_NODE %u\nRTT_us %u\nCHUNK_SIZE %u\n"
               "NUM_SLOTS %u\nNUM_UPDATES %u\nPRINT_MASK %u\n", DEFAULTDATASIZE_impl, gbps,
               SWITCH_BUFFER_impl, SWITCH_PORTS_impl, GPUS_PER_NODE_impl, RTT_impl,
               CHUNK_SIZE_impl, NUM_SLOTS_impl, NUM_UPDATES_impl, PRINT_MASK_impl);
        return true;
    }

    [[maybe_unused]] auto _ = read_env_and_print();
}
const uint32_t &DEFAULTDATASIZE = DEFAULTDATASIZE_impl;
const uint32_t &HOST_NIC = HOST_NIC_impl; // host nic speed in Mbps
const uint32_t &SWITCH_BUFFER = SWITCH_BUFFER_impl; // in bytes, per queue (port)
const int32_t &SWITCH_PORTS = SWITCH_PORTS_impl;
const uint32_t &GPUS_PER_NODE = GPUS_PER_NODE_impl;
const uint32_t &RTT = RTT_impl; // us
const uint32_t &CHUNK_SIZE = CHUNK_SIZE_impl;
const uint32_t &NUM_SLOTS = NUM_SLOTS_impl; // pool size
const uint32_t &PRINT_MASK = PRINT_MASK_impl;
// 1
// 2 worker.cpp lock for tensors
// 3 job arrive, start, finish, placement
// 4 forward backward allreduce timestamp
// 7 collective scheduler logs
// 8 packet tracing
const uint32_t &SWITCHML_PKT_SIZE = DEFAULTDATASIZE;
const uint32_t &NUM_UPDATES = NUM_UPDATES_impl;
const bool &COLLECTIVE_STATISTICS = COLLECTIVE_STATISTICS_impl;

std::hash<std::string> hasher;

inline uint64_t get_key(uint64_t job_id, uint64_t tensor_id) {
    return std::hash<std::string>{}(fmt::format("jid{}tid{}", job_id, tensor_id));
}

inline uint64_t get_key(Tensor *tensor) {
    return std::hash<std::string>{}(fmt::format("jid{}tid{}", tensor->job->id, tensor->tensor_id));
}

int myprintf(std::string format, ...) {
    va_list args;
    va_start(args, format);
    auto size = std::vsnprintf(nullptr, 0, format.c_str(), args);
    std::string s(size + 1, '\0');
    va_start(args, format);
    auto r = std::vsprintf(&s[0], format.c_str(), args);
    cpb.update_variable();
    cpb.stdout_in_for_progress(s);
    va_end(args);
    return r;
}

int myprintf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    auto size = std::vsnprintf(nullptr, 0, format, args);
    std::string s(size + 1, '\0');
    va_start(args, format);
    auto r = std::vsprintf(&s[0], format, args);
    cpb.update_variable();
    cpb.stdout_in_for_progress(s);
    va_end(args);
    return r;
}

int myprintf(const char *format, va_list args, int size) {
    std::string s(size + 1, '\0');
    auto r = std::vsprintf(&s[0], format, args);
    cpb.update_variable();
    cpb.stdout_in_for_progress(s);
    return r;
}

int myprintf(uint32_t type, const char *format, ...) {
    if ((1 << type) & PRINT_MASK) {
        va_list args;
        va_start(args, format);
        auto size = std::vsnprintf(nullptr, 0, format, args);
        va_start(args, format);
        auto r = myprintf(format, args, size);
        va_end(args);
        return r;
    } else return 0;
}

Tensor::Tensor(uint64_t size, Worker *machine, Job *job, simcpp20::simulation<SIM_UNIT> &sim)
        : size(size), machine(machine), job(job), tensor_id(get_id()) {
    key = get_key(job->id, tensor_id);
}

Tensor::Tensor(uint64_t id, uint64_t size, Worker *machine, Job *job,
               simcpp20::simulation<SIM_UNIT> &sim)
        : size(size), machine(machine), job(job), tensor_id(id) {
    key = get_key(job->id, tensor_id);
}

Tensor::Tensor(uint64_t id, uint64_t f, uint64_t b, uint64_t size, Worker *machine, Job *job,
               simcpp20::simulation<SIM_UNIT> &sim)
        : size(size), forward_pass_time(f), backward_pass_time{b}, machine(machine), job(job),
          tensor_id(id) {
    key = get_key(job->id, tensor_id);
}

bool strtobool(const std::string &s) {
    switch (hash_compile_time(s.c_str())) {
        case "True"_hash:
        case "true"_hash:
        case "1"_hash:
        case "Yes"_hash:
        case "yes"_hash:
        case "Y"_hash:
        case "y"_hash:
            return true;
        default:
            return false;
    }
}

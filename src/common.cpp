#include "common.h"
#include <string>
#include "CppProgressBar.h"
#include "job.h"

CppProgressBar cpb;

uint64_t get_key(uint64_t job_id, uint64_t tensor_id){
    return job_id * 1000000 + tensor_id;
}

uint64_t get_key(Tensor *tensor) {
    return tensor->job->id * 1000000 + tensor->tensor_id;
}

int myprintf(const char* format, ...) {
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

int myprintf(const char* format, va_list args, int size) {
    std::string s(size + 1, '\0');
    auto r = std::vsprintf(&s[0], format, args);
    cpb.update_variable();
    cpb.stdout_in_for_progress(s);
    return r;
}

int myprintf(uint32_t type, const char* format, ...) {
    if (type & PRINT_MASK) {
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
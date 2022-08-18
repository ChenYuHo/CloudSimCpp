#ifndef MYTOPOLOGY
#define MYTOPOLOGY
#include "network.h"
#include <unordered_set>
#include <memory>
#include <map>

class Worker;
class Switch;

class MyTopology {
public:
    virtual ~MyTopology() = default;

    virtual vector<Worker *> *workers() = 0;

    virtual vector<Switch *> *switches() = 0;

    virtual unsigned no_of_nodes() const = 0;

    virtual void set_switch_num_updates(
            unsigned int, std::unordered_map<unsigned int, unsigned int>) = 0;

    virtual Route *get_worker_to_tor_path(unsigned) = 0;
    virtual Route * get_switch_single_hop_route(unsigned, unsigned, unsigned, bool) = 0;

    virtual bool accommodate(const std::unordered_set<unsigned>&, const std::unordered_set<unsigned>&) {return false;};

    virtual std::deque<uint64_t> bssi(std::unordered_map<Tensor *, double>) {return {};};
};

#endif
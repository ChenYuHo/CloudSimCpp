#ifndef MYTOPOLOGY
#define MYTOPOLOGY
#include "network.h"
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
            unsigned int, std::map<unsigned int, unsigned int>) = 0;

    virtual const Route *get_worker_to_tor_path(unsigned) = 0;
    virtual const Route * get_switch_single_hop_route(unsigned, unsigned, unsigned, bool) = 0;

    virtual bool accommodate(const std::set<unsigned>&, const std::set<unsigned>&) {return false;};

    virtual std::deque<uint64_t> bssi(std::unordered_map<Tensor *, uint64_t>) {return {};};
};

#endif
#ifndef TOPOLOGY
#define TOPOLOGY
#include "network.h"
#include <memory>
#include <map>

class Worker;
class Switch;

class Topology {
public:
    virtual ~Topology() = default;

    virtual vector<const Route *> *get_paths(int src, int dest) = 0;

    virtual vector<int> *get_neighbours(int src) = 0;

    virtual int no_of_nodes() const { abort(); };

    virtual vector<Worker *> workers() = 0;

    virtual vector<Switch *> switches() = 0;

    virtual void set_switch_num_updates(
            unsigned int, std::map<unsigned int, unsigned int>) = 0;

    virtual const Route *get_worker_to_tor_path(unsigned) = 0;
    virtual const Route * get_switch_single_hop_route(unsigned, unsigned, unsigned, bool) = 0;

    virtual bool accommodate(const std::set<unsigned>&, const std::set<unsigned>&) {return false;};
};

#endif

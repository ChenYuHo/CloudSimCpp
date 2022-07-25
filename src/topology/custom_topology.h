#ifndef CLOUDSIMCPP_CUSTOM_TOPOLOGY_H
#define CLOUDSIMCPP_CUSTOM_TOPOLOGY_H

#include "main.h"
#include "simplequeue.h"
#include "simplepipe.h"
#include "config.h"
#include "network.h"
#include "mytopology.h"
#include "eventlist.h"
#include "switch.h"
#include "worker.h"
#include <ostream>

class Cluster;

/*
 * M>=0 layer hierarchical topology with switches having N ports
 * except for root switch (layer 0), every switch connects to (N-1) downwards and 1 upwards
 */
class CustomTopology : public MyTopology {
public:
    CustomTopology(Cluster *, int no_of_nodes, mem_b queuesize, EventList *ev,
                         unsigned gpus_per_node);

    ~CustomTopology() override;

    void init_network(unsigned);

    Route *get_worker_to_tor_path(unsigned src) override;

    std::vector<Switch*> *switches() override { return &tor_switches; };

    std::vector<Worker*> *workers() override { return &_workers; };

    void set_switch_num_updates(
            unsigned int job_id, map<unsigned int, unsigned int> run_config) override;

    Route * get_switch_single_hop_route(unsigned, unsigned, unsigned, bool) override;

    unsigned no_of_nodes() const override { return _no_of_nodes; };
private:
    Cluster *cluster;

    vector<Switch*> tor_switches{};
    vector<Worker*> _workers{};
//    vector<vector<SimplePipe *> > pipes_tor_worker{};
    vector<vector<SimpleQueue *> > queues_tor_worker{};

//    vector<vector<SimplePipe *> > pipes_worker_tor{};
    vector<vector<SimpleQueue *> > queues_worker_tor{};

    EventList *eventlist; // for constructing worker and switch

    void set_params(int no_of_nodes);

    int K{};
    unsigned _no_of_nodes{};
    mem_b _queuesize;

    std::unordered_map<std::string, Route*> routes{};

    bool accommodate(const std::set<unsigned>&, const std::set<unsigned>&) override;

    std::deque<uint64_t> bssi(std::unordered_map<Tensor *, uint64_t> weights) override;

    SimpleQueue *alloc_queue(uint64_t speed) const;
};

#endif // CLOUDSIMCPP_CUSTOM_TOPOLOGY_H

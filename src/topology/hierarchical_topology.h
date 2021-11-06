#ifndef HIERARCHICAL
#define HIERARCHICAL

#include "main.h"
#include "randomqueue.h"
#include "pipe.h"
#include "config.h"
#include "loggers.h"
#include "network.h"
#include "topology.h"
#include "logfile.h"
#include "eventlist.h"
#include "switch.h"
#include "worker.h"
#include <ostream>

#define HOST_ToR_SWITCH(src) (src/(K-1))

class Cluster;

class HierarchicalTopology : public Topology {
public:
    shared_ptr<Switch> core_switch{};
    vector<Switch*> tor_switches{};
    vector<Worker*> _workers{};
    vector<vector<Pipe *> > pipes_core_tor{};
    vector<vector<Pipe *> > pipes_tor_worker{};
    vector<vector<Queue *> > queues_core_tor{};
    vector<vector<Queue *> > queues_tor_worker{};

    vector<vector<Pipe *> > pipes_worker_tor{};
    vector<vector<Pipe *> > pipes_tor_core{};
    vector<vector<Queue *> > queues_worker_tor{};
    vector<vector<Queue *> > queues_tor_core{};

    Logfile *logfile;
    EventList *eventlist;

    HierarchicalTopology(Cluster *, int no_of_nodes, mem_b queuesize, Logfile *log, EventList *ev);

    ~HierarchicalTopology() override;

    void init_network();

    vector<const Route *> *get_paths(int src, int dest) override;

    void count_queue(Queue *);

    void print_path(std::ofstream &paths, int src, const Route *route);

    vector<int> *get_neighbours(int src) override { return nullptr; };

    int no_of_nodes() const override { return _no_of_nodes; }

    const Route *get_worker_to_tor_path(unsigned src) override;

//    const Route *get_tor_to_worker_path(int src, int dest);

    std::vector<Switch*> switches() override { return tor_switches; };

    std::vector<Worker*> workers() override { return _workers; };

    void set_switch_num_updates(
            unsigned int job_id, map<unsigned int, unsigned int> run_config) override;

    const Route * get_switch_single_hop_route(unsigned, unsigned, unsigned, bool) override;
private:
    Cluster *cluster;

    map<Queue *, int> _link_usage;

    int find_lp_switch(Queue *queue);

    int find_up_switch(Queue *queue);

    int find_core_switch(Queue *queue);

    int find_destination(Queue *queue);

    void set_params(int no_of_nodes);

    int K{}, NC{};
    int _no_of_nodes{};
    mem_b _queuesize;

    Queue *alloc_queue(QueueLogger *queueLogger, uint64_t speed) const;
};

#endif

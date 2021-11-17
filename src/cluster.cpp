#include <vector>
#include "cluster.h"
#include "job.h"
#include "worker.h"
#include "switch.h"
#include "topology/hierarchical_topology.h"

class Worker;
class Switch;

void Cluster::check_if_all_jobs_started() {
    if (!all_jobs_submitted) return;
    for (auto j:jobs) {
        if (j->start_time == std::numeric_limits<uint64_t>::max()) return; // at least one job not started yet
    }
    all_jobs_started = true;
}

void Cluster::check_if_all_jobs_finished() {
    if (!all_jobs_started) return;
    for (auto j:jobs) {
        if (j->finish_time == std::numeric_limits<uint64_t>::max()) return; // at least one job not finished yet
    }
    all_jobs_finished = true;
}

void Cluster::init_topo(int switch_ports, mem_b switch_buffer, unsigned gpus_per_node) {
    _topo = (Topology *) new HierarchicalTopology(this, switch_ports, switch_buffer, nullptr,
                                                  &_eventlist, gpus_per_node);
    workers = _topo->workers();
    for (const auto &w : workers) worker_map[w->id] = w;
    switches = _topo->switches();
    for (const auto &s : switches) switch_map[s->id] = s;
}

Cluster::~Cluster() {
    delete (HierarchicalTopology*)_topo;
}

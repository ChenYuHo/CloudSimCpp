#include "custom_topology.h"
#include <vector>
#include <iostream>
#include <algorithm>
#include "job.h"
#include "common.h"
#include <cfloat>
#include <glog/logging.h>

CustomTopology::CustomTopology
        (Cluster *c, int switch_ports, mem_b queuesize, EventList *ev, unsigned gpus_per_node)
        : cluster(c), _queuesize(queuesize), eventlist(ev), _no_of_nodes(switch_ports) {
    tor_switches.resize(1, nullptr);
    _workers.resize(_no_of_nodes, nullptr);
//    pipes_worker_tor.resize(_no_of_nodes, vector<SimplePipe *>(1));
//    pipes_tor_worker.resize(1, vector<SimplePipe *>(_no_of_nodes));
    queues_tor_worker.resize(1, vector<SimpleQueue *>(_no_of_nodes));
    queues_worker_tor.resize(_no_of_nodes, vector<SimpleQueue *>(1));
    init_network(gpus_per_node);
}


inline SimpleQueue *CustomTopology::alloc_queue(uint64_t speed = HOST_NIC) const {
    return new SimpleQueue(speedFromMbps(speed), _queuesize, *eventlist);
}

void CustomTopology::init_network(unsigned gpus_per_node) {
    for (int j = 0; j < 1; j++)
        for (int k = 0; k < _no_of_nodes; k++) {
            queues_tor_worker[j][k] = nullptr;
//            pipes_tor_worker[j][k] = nullptr;
            queues_worker_tor[k][j] = nullptr;
//            pipes_worker_tor[k][j] = nullptr;
        }

    // links from ToR switch to worker
    for (int k = 0; k < _no_of_nodes; ++k) {
        // Downlink
        queues_tor_worker[0][k] = alloc_queue();
        queues_tor_worker[0][k]->setName(fmt::format("ToR0->SERVER{}", k));
//        pipes_tor_worker[0][k] = new SimplePipe(timeFromUs(RTT), *eventlist);
//        pipes_tor_worker[0][k]->setName(fmt::format("SimplePipe-ToR0->SERVER{}", k));

        // Uplink
        queues_worker_tor[k][0] = alloc_queue();
        queues_worker_tor[k][0]->setName(fmt::format("SERVER{}->ToR0", k));
//        pipes_worker_tor[k][0] = new SimplePipe(timeFromUs(RTT), *eventlist);
//        pipes_worker_tor[k][0]->setName(fmt::format("SimplePipe-SERVER{}->ToR0", k));
    }

    // instantiate workers and switches
    tor_switches[0] = new Switch(0, *eventlist, cluster, nullptr);
    for (int k = 0; k < _no_of_nodes; k++) {
        auto route_to_tor = new Route();
        route_to_tor->push_back(queues_worker_tor[k][0]);
//    route_out->push_back(pipes_worker_tor[src][dest]);
        route_to_tor->push_back(tor_switches[0]);
        route_to_tor->non_null();
        _workers[k] = new Worker(*eventlist, cluster, tor_switches[0], gpus_per_node, route_to_tor);
        tor_switches[0]->machines.push_back(_workers[k]);
    }
}

Route *CustomTopology::get_worker_to_tor_path(unsigned src) {
    auto key = fmt::format("w{}", src);
//    if (routes.contains(key)) {
//        return routes[key];
//    }
    auto route_out = new Route();
    route_out->push_back(queues_worker_tor[src][0]);
//    route_out->push_back(pipes_worker_tor[src][0]);
    route_out->push_back(tor_switches[0]);
    route_out->non_null();
//    routes[key] = route_out;
    return route_out;
}

void CustomTopology::set_switch_num_updates(
        unsigned int job_id, unordered_map<unsigned int, unsigned int> run_config) {
    bool cleaner_worker_set;
    for (const auto &pair: run_config) {
        if (!cleaner_worker_set) {
            cleaner_worker_set = true;
            // this worker is responsible for cleaning the switch after job completes
            _workers[pair.first]->clean_ToR_for_job[job_id] = true;
        }
        auto &tor_num_updates_for_job = tor_switches[0]->num_updates_for_job;
        auto &tor_downward_ids_for_job = tor_switches[0]->downward_ids_for_job;
        if (tor_num_updates_for_job.find(job_id) == tor_num_updates_for_job.end()) tor_num_updates_for_job[job_id] = 0;
        tor_num_updates_for_job[job_id] += 1;
        tor_downward_ids_for_job[job_id].insert(pair.first);

    }
    auto &map = tor_switches[0]->num_updates_for_job;
    auto &map_ids = tor_switches[0]->downward_ids_for_job;
    myprintf("ToR %d Jid %d num_updates %d\n", 0, job_id, map[job_id]);
    auto str = fmt::format("ToR 0 Jid {} downward: ", job_id);
    for (const auto &p: map_ids[job_id]) {
        str += fmt::format("{} ", p);
    }
    str += "\n";
    myprintf(str);
    tor_switches[0]->top_level_for_job[job_id] = true;
    myprintf("Job %u spans within ToR switch 0\n", job_id);

}

Route *CustomTopology::get_switch_single_hop_route(unsigned src, unsigned layer,
                                                         unsigned dest, bool upward) {
    // from ToR to workers
    auto key = fmt::format("s{}l0u0d{}", src, dest);
//    if (routes.contains(key)) {
//        return routes[key];
//    }
    auto route_out = new Route();
    route_out->push_back(queues_tor_worker[src][dest]);
//    route_out->push_back(pipes_tor_worker[src][dest]);
    route_out->push_back(_workers[dest]);
    route_out->non_null();
//    routes[key] = route_out;
    return route_out;
}

bool CustomTopology::accommodate(const std::unordered_set<unsigned> &these, const std::unordered_set<unsigned> &those) {
    return false;
}

CustomTopology::~CustomTopology() {
    LOG(INFO) << "cleaning CustomTopology";
    for (auto p: tor_switches) {
//        printf("%p\n", p);
        delete p;
    }
    tor_switches.clear();
    for (auto p: _workers) delete p;
    _workers.clear();

    for (int k = 0; k < _no_of_nodes; k++) {
        delete queues_tor_worker[0][k];
//        delete pipes_tor_worker[0][k];
        delete queues_worker_tor[k][0];
//        delete pipes_worker_tor[k][0];
    }

//    for (const auto &pair: routes) {
//        delete pair.second;
//    }
//    routes.clear();
//    delete logfile;
//    delete eventlist;
}

struct DoubleDefaultedToOne {
    double d = 1;
};

std::deque<uint64_t> CustomTopology::bssi(std::unordered_map<Tensor *, double> weights) {
    // coflow (per job) -> weight
    std::deque<uint64_t> result{};
    auto iters = int(weights.size());
    for (int i = 0; i < iters - 1; ++i) {
        std::unordered_map<unsigned, std::unordered_map<unsigned, DoubleDefaultedToOne>> data_port_coflow{};
        // port (per worker), coflow -> data
        std::vector<DoubleDefaultedToOne> data_port(_no_of_nodes);
        // Find the most bottlenecked port
        unsigned bottlenecked; // wid
        double current_max = 0;
        for (auto &pair: weights) {
            auto &tensor = pair.first;
            for (auto wid: tensor->job->wids_allocated) {
                auto data = double((CHUNK_SIZE == 0) ? tensor->size
                                                     : std::min(tensor->size - tensor->allreduced_size, CHUNK_SIZE));
                data_port_coflow[wid][tensor->job->id].d += data;
                data_port[wid].d += data;
                if (data_port[wid].d >= current_max) {
                    current_max = data_port[wid].d;
                    bottlenecked = wid;
                }
            }
        }
        DVLOG(3) << "bottlenecked port " << bottlenecked;
        // Select weighted largest job to schedule last
        Tensor *weighted_largest;
        auto current_min = DBL_MAX;
        double min_weight;
        for (auto &pair: weights) {
            auto weight = pair.second / data_port_coflow[bottlenecked][pair.first->job->id].d;
            if (weight <= current_min) {
                current_min = weight;
                weighted_largest = pair.first;
                min_weight = pair.second;
            }
        }
        result.push_front(weighted_largest->key);

        // Scale the weights
        auto s = data_port_coflow[bottlenecked][weighted_largest->job->id].d;
        weights.erase(weighted_largest);
        for (auto &pair: weights) {
            pair.second -= (min_weight * data_port_coflow[bottlenecked][pair.first->job->id].d / s);
        }
    }
    result.push_front(weights.begin()->first->key);
    return result;
}

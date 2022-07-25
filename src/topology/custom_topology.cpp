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

    // instantiate workers and switches
    tor_switches[0] = new Switch(0, *eventlist, cluster, nullptr);
    for (int k = 0; k < _no_of_nodes; k++) {
        _workers[k] = new Worker(*eventlist, cluster, tor_switches[0], gpus_per_node);
        tor_switches[0]->machines.push_back(_workers[k]);
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

}

const Route *CustomTopology::get_worker_to_tor_path(unsigned src) {
    auto key = fmt::format("w{}", src);
    if (routes.contains(key)) {
        return routes[key];
    }
    auto route_out = new Route();
    route_out->push_back(queues_worker_tor[src][0]);
//    route_out->push_back(pipes_worker_tor[src][0]);
    route_out->push_back(tor_switches[0]);
    route_out->non_null();
    routes[key] = route_out;
    return route_out;
}

void CustomTopology::set_switch_num_updates(
        unsigned int job_id, map<unsigned int, unsigned int> run_config) {
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

const Route *CustomTopology::get_switch_single_hop_route(unsigned src, unsigned layer,
                                                         unsigned dest, bool upward) {
    // from ToR to workers
    auto key = fmt::format("s{}l0u0d{}", src, dest);
    if (routes.contains(key)) {
        return routes[key];
    }
    auto route_out = new Route();
    route_out->push_back(queues_tor_worker[src][dest]);
//    route_out->push_back(pipes_tor_worker[src][dest]);
    route_out->push_back(_workers[dest]);
    route_out->non_null();
    routes[key] = route_out;
    return route_out;
}

bool CustomTopology::accommodate(const std::set<unsigned> &these, const std::set<unsigned> &those) {
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

    for (const auto &pair: routes) {
        delete pair.second;
    }
    routes.clear();
//    delete logfile;
//    delete eventlist;
}

std::deque<uint64_t> CustomTopology::bssi(std::unordered_map<Tensor *, uint64_t> weights) {
//    myprintf("before ordering: ");
//    for (auto& pair: weights) {
//        myprintf("jid %u tkey %u ", pair.first->job->id, pair.first->key);
//    }
//    myprintf("\n");
    // coflow (per job) -> weight
    std::unordered_map<unsigned, std::unordered_map<unsigned, unsigned>> data_port_coflow; // port (per worker), coflow -> data
    std::vector<unsigned> data_port(_no_of_nodes);
    std::deque<uint64_t> result{};
    auto iters = weights.size();
    if (iters == 1) {
        for (auto &pair: weights) {
            result.push_front(pair.first->key);
        }
        return result;
    }
    for (unsigned i = 0; i < iters; ++i) {
//        myprintf("iter %u/%u\n", i+1, iters);
        // Find the most bottlenecked port
        unsigned bottlenecked; // wid
        unsigned current_max = 0;
        for (auto &pair: weights) {
            auto &tensor = pair.first;
            for (auto wid: tensor->job->wids_allocated) {
                auto data = ((CHUNK_SIZE == 0)
                             ? tensor->size
                             : (tensor->size - tensor->allreduced_size > CHUNK_SIZE)
                               ? CHUNK_SIZE
                               : tensor->size - tensor->allreduced_size);
                data_port_coflow[wid][tensor->job->id] += data;
                data_port[wid] += data;
                if (data_port[wid] >= current_max) {
                    current_max = data_port[wid];
                    bottlenecked = wid;
                }
            }
        }
//        for (auto& p: data_port_coflow) {
//            for (auto & pp: p.second) {
//                myprintf("port %u coflow %u data %u\n", p.first, pp.first, pp.second);
//            }
//        }
//        myprintf("bottleneck port %u\n", bottlenecked);
        // Select weighted largest job to schedule last
        Tensor *weighted_largest;
        auto current_min = DBL_MAX;
        double min_weight;
        for (auto &pair: weights) {
//            myprintf("tid %llu bport %u jid %u dpc %u\n", pair.second, bottlenecked, pair.first->job->id, data_port_coflow[bottlenecked][pair.first->job->id]);
            if (data_port_coflow[bottlenecked][pair.first->job->id] == 0) continue;
            auto weight = pair.second / data_port_coflow[bottlenecked][pair.first->job->id];
            if (weight <= current_min) {
                current_min = weight;
                weighted_largest = pair.first;
                min_weight = pair.second;
            }
        }
        result.push_front(weighted_largest->key);

        // Scale the weights
        for (auto &pair: weights) {
            if (pair.first->job->id != weighted_largest->job->id) {
                pair.second -= (min_weight * data_port_coflow[bottlenecked][pair.first->job->id] /
                                data_port_coflow[bottlenecked][weighted_largest->job->id]);
            }
        }
        weights.erase(weighted_largest);
    }
//    myprintf("result:");
//    for (auto i: result) myprintf("%u ", i);
//    myprintf("\n");
    return result;
}

//void CustomTopology::register_switch(Switch *s) {
//    myprintf("register %d %p\n", s->id(), s);
////    cout<<s->id<<s;
//    tor_switches[s->id()] = s;
//}
//
//void CustomTopology::register_core_switch(Switch *s) {
//    core_switch[s->id()] = s;
//}
//
//void CustomTopology::register_worker(Worker *w) {
//    _workers[w->id()] = w;
//}

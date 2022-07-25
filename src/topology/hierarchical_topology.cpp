#include "hierarchical_topology.h"
#include <vector>
#include <memory>
#include <iostream>
#include <algorithm>
#include "job.h"
#include <cfloat>
#include <glog/logging.h>

template<typename T>
string toa(T n) {
    stringstream s;
    s << n;
    return s.str();
}

HierarchicalTopology::HierarchicalTopology
        (Cluster *c, int switch_ports, mem_b queuesize, EventList *ev, unsigned gpus_per_node, unsigned num_layers)
        : cluster(c), _queuesize(queuesize), eventlist(ev), num_layers(num_layers) {
    LOG_IF(WARNING, num_layers > 2) << "num_layer > 2 not yet implemented";
    set_params(switch_ports);
    init_network(gpus_per_node);
}

void HierarchicalTopology::set_params(int switch_ports) {
    LOG_IF(FATAL, switch_ports < 1) << "can't have non-positive switch_ports";
    _no_of_nodes = switch_ports * (switch_ports - 1);
    K = switch_ports; // Switches with K number of ports, K-ary or K-port Fat tree
    tor_switches.resize(K, nullptr);
    _workers.resize(_no_of_nodes, nullptr);
//    pipes_tor_core.resize(K, vector<SimplePipe *>(1));
//    pipes_core_tor.resize(1, vector<SimplePipe *>(K));
//    pipes_worker_tor.resize(_no_of_nodes, vector<SimplePipe *>(K));
//    pipes_tor_worker.resize(K, vector<SimplePipe *>(_no_of_nodes));
    queues_tor_worker.resize(K, vector<SimpleQueue *>(_no_of_nodes));
    queues_worker_tor.resize(_no_of_nodes, vector<SimpleQueue *>(K));
    queues_tor_core.resize(K, vector<SimpleQueue *>(1));
    queues_core_tor.resize(1, vector<SimpleQueue *>(K));
}

inline SimpleQueue *HierarchicalTopology::alloc_queue(uint64_t speed = HOST_NIC) const {
    return new SimpleQueue(speedFromMbps(speed), _queuesize, *eventlist);
}

void HierarchicalTopology::init_network(unsigned gpus_per_node) {
    for (int j = 0; j < 1; j++)
        for (int k = 0; k < K; k++) {
            queues_core_tor[j][k] = nullptr;
//            pipes_core_tor[j][k] = nullptr;
        }

    for (int j = 0; j < K; j++)
        for (int k = 0; k < 1; k++) {
            queues_tor_core[j][k] = nullptr;
//            pipes_tor_core[j][k] = nullptr;
        }

    for (int j = 0; j < K; j++)
        for (int k = 0; k < K * (K - 1); k++) {
            queues_tor_worker[j][k] = nullptr;
//            pipes_tor_worker[j][k] = nullptr;
            queues_worker_tor[k][j] = nullptr;
//            pipes_worker_tor[k][j] = nullptr;
        }

    // links from ToR switch to worker
    for (int j = 0; j < K; j++) {
        for (int l = 0; l < K - 1; l++) {
            int k = j * (K - 1) + l;
            // Downlink
            queues_tor_worker[j][k] = alloc_queue();
            queues_tor_worker[j][k]->setName(fmt::format("ToR0->SERVER{}", k));
//            pipes_tor_worker[j][k] = new SimplePipe(timeFromUs(RTT), *eventlist);
//            pipes_tor_worker[j][k]->setName(fmt::format("SimplePipe-ToR0->SERVER{}", k));

            // Uplink
            queues_worker_tor[k][j] = alloc_queue();
            queues_worker_tor[k][j]->setName(fmt::format("SERVER{}->ToR0", k));
//            pipes_worker_tor[k][j] = new SimplePipe(timeFromUs(RTT), *eventlist);
//            pipes_worker_tor[k][j]->setName(fmt::format("SimplePipe-SERVER{}->ToR0", k));
        }
    }

    // ToR to core!
    for (int j = 0; j < K; j++) {
        int k = 0;
        // Downlink
        queues_tor_core[j][k] = alloc_queue();
        queues_tor_core[j][k]->setName("ToR" + toa(j) + "->Core" + toa(k));
//        pipes_tor_core[j][k] = new SimplePipe(timeFromUs(RTT), *eventlist);
//        pipes_tor_core[j][k]->setName("SimplePipe-ToR" + toa(j) + "->Core" + toa(k));

        // Uplink
        queues_core_tor[k][j] = alloc_queue();
        queues_core_tor[k][j]->setName("Core" + toa(k) + "->ToR" + toa(j));
//        pipes_core_tor[k][j] = new SimplePipe(timeFromUs(RTT), *eventlist);
//        pipes_core_tor[k][j]->setName("SimplePipe-Core" + toa(k) + "->ToR" + toa(j));
    }


    // instantiate workers and switches
    core_switch = make_unique<Switch>(K, *eventlist, cluster, nullptr);
    core_switch->layer = 1;
    for (int j = 0; j < K; j++) {
        auto route_to_core = new Route();
        route_to_core->push_back(queues_tor_core[j][0]);
        route_to_core->push_back(core_switch.get());
        route_to_core->non_null();
        tor_switches[j] = new Switch(j, *eventlist, cluster, core_switch.get(), route_to_core);
//        printf("%p\n", tor_switches[j]);
        for (int k = 0; k < K - 1; k++) {
            auto route_to_tor = new Route();
            route_to_tor->push_back(queues_worker_tor[k + j * (K - 1)][j]);
//    route_out->push_back(pipes_worker_tor[src][dest]);
            route_to_tor->push_back(tor_switches[j]);
            route_to_tor->non_null();
            _workers[k + j * (K - 1)] = new Worker(*eventlist, cluster, tor_switches[j], gpus_per_node, route_to_tor);
            tor_switches[j]->machines.push_back(_workers[k + j * (K - 1)]);
        }
    }
}


Route *HierarchicalTopology::get_worker_to_tor_path(unsigned src) {
    auto dest = HOST_ToR_SWITCH(src);
    auto key = fmt::format("ws{}d{}", src, dest);
    if (routes.contains(key)) {
        return routes[key];
    }
    auto route_out = new Route();
    route_out->push_back(queues_worker_tor[src][dest]);
//    route_out->push_back(pipes_worker_tor[src][dest]);
    route_out->push_back(tor_switches[dest]);
    route_out->non_null();
    routes[key] = route_out;
    return route_out;
}

void HierarchicalTopology::set_switch_num_updates(
        unsigned int job_id, map<unsigned int, unsigned int> run_config) {
    std::set<unsigned> involved_tors{};
    unordered_map<unsigned, bool> cleaner_worker_set{};
    for (const auto &pair: run_config) {
        auto tor_id = HOST_ToR_SWITCH(pair.first);
        if (!cleaner_worker_set[tor_id]) {
            cleaner_worker_set[tor_id] = true;
            // this worker is responsible for cleaning the switch after job completes
            _workers[pair.first]->clean_ToR_for_job[job_id] = true;
        }
        involved_tors.insert(tor_id);
        auto &map = tor_switches[tor_id]->num_updates_for_job;
        auto &map_ids = tor_switches[tor_id]->downward_ids_for_job;
        if (map.find(job_id) == map.end()) map[job_id] = 0;
        map[job_id] += 1;
        map_ids[job_id].insert(pair.first);

    }
    for (auto &tor_id: involved_tors) {
        auto &map = tor_switches[tor_id]->num_updates_for_job;
        auto &map_ids = tor_switches[tor_id]->downward_ids_for_job;
        myprintf("ToR %d Jid %d num_updates %d\n", tor_id, job_id, map[job_id]);
        auto str = fmt::format("ToR {} Jid {} downward: ", tor_id, job_id);
        for (const auto &p: map_ids[job_id]) {
            str += fmt::format("{} ", p);
        }
        str += "\n";
        myprintf(str);
    }
    if (involved_tors.size() > 1) {
        // need to involve the core switch
        core_switch->top_level_for_job[job_id] = true;
        bool cleaner_set = false;
        std::string tors;
        for (auto &tor_id: involved_tors) {
            tors += "," + std::to_string(tor_id);
            if (!cleaner_set) {
                cleaner_set = true;
                tor_switches[tor_id]->clean_upper_level_switch_for_job[job_id] = true;
            }
            tor_switches[tor_id]->top_level_for_job[job_id] = false;
        }
        myprintf("Job %u spans across multiple ToR switches: %s\n", job_id, tors.substr(1).c_str());
        core_switch->num_updates_for_job[job_id] = involved_tors.size();
        core_switch->downward_ids_for_job[job_id].merge(involved_tors);
        myprintf("core Jid %d num_updates %lu\n", job_id, core_switch->num_updates_for_job[job_id]);
        auto str = fmt::format("core Jid {} downward: ", job_id);
        for (const auto &p: core_switch->downward_ids_for_job[job_id]) {
            str += fmt::format("{} ", p);
        }
        str += "\n";
        myprintf(str);
    } else {
        core_switch->top_level_for_job[job_id] = false;
        std::string tor;
        for (auto &tor_id: involved_tors) {
            tor += std::to_string(tor_id);
            tor_switches[tor_id]->top_level_for_job[job_id] = true;
        }
        myprintf("Job %u spans within ToR switch %s\n", job_id, tor.c_str());
    }
}

Route *HierarchicalTopology::get_switch_single_hop_route(unsigned src, unsigned layer,
                                                         unsigned dest, bool upward) {
    // valid layer: 0, 1
    assert(layer == 1 || layer == 0);
//    auto route_out = new Route();
    if (layer == 1) { // from core to ToRs
        src = 0;
        auto key = fmt::format("{}s,dl1u0{}", src, dest);
        auto route_out = new Route();
        route_out->push_back(queues_core_tor[src][dest]);
//        route_out->push_back(pipes_core_tor[src][dest]);
        route_out->push_back(tor_switches[dest]);
        route_out->non_null();
        return route_out;
    } else if (upward) { // from ToR to core
        dest = 0;
        auto key = fmt::format("{}s,dl0u1{}", src, dest);
        auto route_out = new Route();
        route_out->push_back(queues_tor_core[src][dest]);
//        route_out->push_back(pipes_tor_core[src][dest]);
        route_out->push_back(core_switch.get());
        route_out->non_null();
        return route_out;
    } else { // from ToR to workers
        auto key = fmt::format("{}s,dl0u0{}", src, dest);
        auto route_out = new Route();
        route_out->push_back(queues_tor_worker[src][dest]);
//        route_out->push_back(pipes_tor_worker[src][dest]);
        route_out->push_back(_workers[dest]);
        route_out->non_null();
        return route_out;
    }

}

bool HierarchicalTopology::accommodate(const std::set<unsigned> &these, const std::set<unsigned> &those) {
    std::set<unsigned> tors_these;
    for (auto wid: these) tors_these.insert(HOST_ToR_SWITCH(wid));
    if (tors_these.size() == 1) { // don't need core
        tors_these.clear();
    }
    std::set<unsigned> tors_those;
    for (auto wid: those) {
        if (these.contains(wid))
            return false;
        tors_those.insert(HOST_ToR_SWITCH(wid));
    }
    if (tors_those.size() == 1) { // don't need core
        tors_those.clear();
    }
    if (ranges::any_of(tors_those.cbegin(), tors_those.cend(),
                       [&tors_these](unsigned tor_id) { return tors_these.contains(tor_id); })) {
        return false;
    }
    return true;
}

HierarchicalTopology::~HierarchicalTopology() {
    for (auto p: tor_switches) {
//        printf("%p\n", p);
        delete p;
    }
    tor_switches.clear();
    for (auto p: _workers) delete p;
    _workers.clear();

    for (int j = 0; j < 1; j++) {
        for (int k = 0; k < K; k++) {
            delete queues_core_tor[j][k];
//            delete pipes_core_tor[j][k];
        }
    }

    for (int j = 0; j < K; j++) {
        for (int k = 0; k < 1; k++) {
            delete queues_tor_core[j][k];
//            delete pipes_tor_core[j][k];
        }
    }

    for (int j = 0; j < K; j++) {
        for (int k = 0; k < K * (K - 1); k++) {
            delete queues_tor_worker[j][k];
//            delete pipes_tor_worker[j][k];
            delete queues_worker_tor[k][j];
//            delete pipes_worker_tor[k][j];
        }
    }
//    delete logfile;
//    delete eventlist;
}

std::deque<uint64_t> HierarchicalTopology::bssi(std::unordered_map<Tensor *, uint64_t> weights) {
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

//void HierarchicalTopology::register_switch(Switch *s) {
//    myprintf("register %d %p\n", s->id(), s);
////    cout<<s->id<<s;
//    tor_switches[s->id()] = s;
//}
//
//void HierarchicalTopology::register_core_switch(Switch *s) {
//    core_switch[s->id()] = s;
//}
//
//void HierarchicalTopology::register_worker(Worker *w) {
//    _workers[w->id()] = w;
//}

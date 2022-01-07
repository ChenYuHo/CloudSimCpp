#include "hierarchical_topology.h"
#include <vector>
#include <memory>
#include <iostream>
#include <algorithm>
#include "main.h"
#include "queue.h"
#include "job.h"
#include <cfloat>

template<typename T>
string toa(T n) {
    stringstream s;
    s << n;
    return s.str();
}

HierarchicalTopology::HierarchicalTopology(Cluster *c, int switch_ports,
                                           mem_b queuesize, Logfile *lg,
                                           EventList *ev, unsigned gpus_per_node)
        : cluster(c) {
    _queuesize = queuesize;
    logfile = lg;
    eventlist = ev;
    set_params(switch_ports);
    init_network(gpus_per_node);
}

void HierarchicalTopology::set_params(int switch_ports) {
    assert(switch_ports>1);
    _no_of_nodes = switch_ports * (switch_ports-1);
    K = switch_ports; // Switches with K number of ports, K-ary or K-port Fat tree
//    while (_no_of_nodes < no_of_nodes) {
//        K++;
//        _no_of_nodes = K * (K - 1);
//    }
//    if (_no_of_nodes > no_of_nodes) {
//        cerr << "Topology Error: can't have a two-layer Hierarchical Topology with " << no_of_nodes
//             << " nodes\n";
//        exit(1);
//    }
    tor_switches.resize(K, nullptr);
    _workers.resize(_no_of_nodes, nullptr);
    pipes_tor_core.resize(K, vector<Pipe *>(1));
    pipes_core_tor.resize(1, vector<Pipe *>(K));
    pipes_worker_tor.resize(_no_of_nodes, vector<Pipe *>(K));
    pipes_tor_worker.resize(K, vector<Pipe *>(_no_of_nodes));
    queues_tor_worker.resize(K, vector<Queue *>(_no_of_nodes));
    queues_worker_tor.resize(_no_of_nodes, vector<Queue *>(K));
    queues_tor_core.resize(K, vector<Queue *>(1));
    queues_core_tor.resize(1, vector<Queue *>(K));
}

Queue *HierarchicalTopology::alloc_queue(QueueLogger *queueLogger, uint64_t speed = HOST_NIC) const {
    return new RandomQueue(speedFromMbps(speed),
                           _queuesize,
//                           memFromPkt(SWITCH_BUFFER + RANDOM_BUFFER),
                           *eventlist, queueLogger, memFromPkt(RANDOM_BUFFER));
}

void HierarchicalTopology::init_network(unsigned gpus_per_node) {
    for (int j = 0; j < 1; j++)
        for (int k = 0; k < K; k++) {
            queues_core_tor[j][k] = nullptr;
            pipes_core_tor[j][k] = nullptr;
        }

    for (int j = 0; j < K; j++)
        for (int k = 0; k < 1; k++) {
            queues_tor_core[j][k] = nullptr;
            pipes_tor_core[j][k] = nullptr;
        }

    for (int j = 0; j < K; j++)
        for (int k = 0; k < K * (K - 1); k++) {
            queues_tor_worker[j][k] = nullptr;
            pipes_tor_worker[j][k] = nullptr;
            queues_worker_tor[k][j] = nullptr;
            pipes_worker_tor[k][j] = nullptr;
        }

    // instantiate workers and switches
    core_switch = make_unique<Switch>(K, *eventlist, cluster, nullptr);
    core_switch->layer = 1;
    for (int j = 0; j < K; j++) {
        tor_switches[j] = new Switch(j, *eventlist, cluster, core_switch.get());
//        printf("%p\n", tor_switches[j]);
        for (int k = 0; k < K - 1; k++) {
            _workers[k + j * (K - 1)] = new Worker(*eventlist, cluster, tor_switches[j], gpus_per_node);
            tor_switches[j]->machines.push_back(_workers[k + j * (K - 1)]);
        }
    }


    // links from ToR switch to worker
    for (int j = 0; j < K; j++) {
        for (int l = 0; l < K - 1; l++) {
            int k = j * (K - 1) + l;
            // Downlink
            queues_tor_worker[j][k] = alloc_queue(nullptr);
            queues_tor_worker[j][k]->setName("ToR" + toa(j) + "->SERVER" + toa(k));
            pipes_tor_worker[j][k] = new Pipe(timeFromUs(RTT), *eventlist);
            pipes_tor_worker[j][k]->setName("Pipe-ToR" + toa(j) + "->SERVER" + toa(k));

            // Uplink
            queues_worker_tor[k][j] = alloc_queue(nullptr);
            queues_worker_tor[k][j]->setName("SERVER" + toa(k) + "->ToR" + toa(j));
            pipes_worker_tor[k][j] = new Pipe(timeFromUs(RTT), *eventlist);
            pipes_worker_tor[k][j]->setName("Pipe-SERVER" + toa(k) + "->ToR" + toa(j));
        }
    }

    // ToR to core!
    for (int j = 0; j < K; j++) {
        int k = 0;
        // Downlink
        queues_tor_core[j][k] = alloc_queue(nullptr);
        queues_tor_core[j][k]->setName("ToR" + toa(j) + "->Core" + toa(k));
        pipes_tor_core[j][k] = new Pipe(timeFromUs(RTT), *eventlist);
        pipes_tor_core[j][k]->setName("Pipe-ToR" + toa(j) + "->Core" + toa(k));

        // Uplink
        queues_core_tor[k][j] = alloc_queue(nullptr);
        queues_core_tor[k][j]->setName("Core" + toa(k) + "->ToR" + toa(j));
        pipes_core_tor[k][j] = new Pipe(timeFromUs(RTT), *eventlist);
        pipes_core_tor[k][j]->setName("Pipe-Core" + toa(k) + "->ToR" + toa(j));
    }

}

void check_non_null(Route *rt) {
    int fail = 0;
    for (unsigned int i = 1; i < rt->size() - 1; i += 2)
        if (rt->at(i) == nullptr) {
            fail = 1;
            break;
        }

    if (fail) {
        //    cout <<"Null queue in route"<<endl;
        for (unsigned int i = 1; i < rt->size() - 1; i += 2)
            myprintf("%p ", rt->at(i));

        cout << endl;
        assert(0);
    }
}

const Route *HierarchicalTopology::get_worker_to_tor_path(unsigned src) {
    auto dest = HOST_ToR_SWITCH(src);
    auto key = "ws"+std::to_string(src)+"d"+std::to_string(dest);
    if (routes.contains(key)) {
        return routes[key];
    }
    auto route_out = new Route();
    route_out->push_back(queues_worker_tor[src][dest]);
    route_out->push_back(pipes_worker_tor[src][dest]);
    route_out->push_back(tor_switches[dest]);
    check_non_null(route_out);
    routes[key] = route_out;
    return route_out;
}

//const Route *HierarchicalTopology::get_tor_to_worker_path(int src, int dest) {
//    route_t *route_out;
//    if (HOST_ToR_SWITCH(dest) == src) {
//        route_out = new Route();
//        route_out->push_back(queues_tor_worker[src][dest]);
//        route_out->push_back(pipes_tor_worker[src][dest]);
//        route_out->push_back(_workers[dest]);
//        check_non_null(route_out);
//    }
//    return route_out;
//}

// unused
vector<const Route *> *HierarchicalTopology::get_paths(int src, int dest) {
    auto *paths = new vector<const Route *>();

    route_t *routeout, *routeback;
    //QueueLoggerSimple *simplequeuelogger = new QueueLoggerSimple();
    //QueueLoggerSimple *simplequeuelogger = 0;
    //logfile->addLogger(*simplequeuelogger);
    //Queue* pqueue = new Queue(speedFromMbps((uint64_t)HOST_NIC), memFromPkt(FEEDER_BUFFER), *eventlist, simplequeuelogger);
    //pqueue->setName("PQueue_" + toa(src) + "_" + toa(dest));
    //logfile->writeName(*pqueue);
    if (HOST_ToR_SWITCH(src) == HOST_ToR_SWITCH(dest)) {

        // forward path
        routeout = new Route();
        //routeout->push_back(pqueue);
        routeout->push_back(queues_worker_tor[src][HOST_ToR_SWITCH(src)]);
        routeout->push_back(pipes_worker_tor[src][HOST_ToR_SWITCH(src)]);

//        routeout->push_back(tor_switches[HOST_ToR_SWITCH(src)]);

        routeout->push_back(queues_tor_worker[HOST_ToR_SWITCH(dest)][dest]);
        routeout->push_back(pipes_tor_worker[HOST_ToR_SWITCH(dest)][dest]);

        // reverse path for RTS packets
        routeback = new Route();
        routeback->push_back(queues_worker_tor[dest][HOST_ToR_SWITCH(dest)]);
        routeback->push_back(pipes_worker_tor[dest][HOST_ToR_SWITCH(dest)]);

//        routeback->push_back(tor_switches[HOST_ToR_SWITCH(dest)]);

        routeback->push_back(queues_tor_worker[HOST_ToR_SWITCH(src)][src]);
        routeback->push_back(pipes_tor_worker[HOST_ToR_SWITCH(src)][src]);

        routeout->set_reverse(routeback);
        routeback->set_reverse(routeout);
//        cout<<"print route\n";
//        print_route(*routeout);
        paths->push_back(routeout);

        check_non_null(routeout);
        return paths;
    } else {
        int core = 0;
        routeout = new Route();
        //routeout->push_back(pqueue);
        routeout->push_back(queues_worker_tor[src][HOST_ToR_SWITCH(src)]);
        routeout->push_back(pipes_worker_tor[src][HOST_ToR_SWITCH(src)]);

        routeout->push_back(queues_tor_core[HOST_ToR_SWITCH(src)][core]);
        routeout->push_back(pipes_tor_core[HOST_ToR_SWITCH(src)][core]);

        //now take the only link down to the destination worker!

//        int HOST_ToR_SWITCH(dest) = HOST_POD(dest) * K / 2 + 2 * core / K;
        //myprintf("K %d HOST_POD(%d) %d core %d HOST_ToR_SWITCH(dest) %d\n",K,dest,HOST_POD(dest),core, HOST_ToR_SWITCH(dest));

        routeout->push_back(queues_core_tor[core][HOST_ToR_SWITCH(dest)]);
        routeout->push_back(pipes_core_tor[core][HOST_ToR_SWITCH(dest)]);

        routeout->push_back(queues_tor_worker[HOST_ToR_SWITCH(dest)][dest]);
        routeout->push_back(pipes_tor_worker[HOST_ToR_SWITCH(dest)][dest]);

        // reverse path for RTS packets
        routeback = new Route();

        routeback->push_back(queues_worker_tor[dest][HOST_ToR_SWITCH(dest)]);
        routeback->push_back(pipes_worker_tor[dest][HOST_ToR_SWITCH(dest)]);

        routeback->push_back(queues_tor_core[HOST_ToR_SWITCH(dest)][core]);
        routeback->push_back(pipes_tor_core[HOST_ToR_SWITCH(dest)][core]);

        //now take the only link back down to the src worker!

        routeback->push_back(queues_core_tor[core][HOST_ToR_SWITCH(src)]);
        routeback->push_back(pipes_core_tor[core][HOST_ToR_SWITCH(src)]);
        routeback->push_back(queues_tor_worker[HOST_ToR_SWITCH(src)][src]);
        routeback->push_back(pipes_tor_worker[HOST_ToR_SWITCH(src)][src]);


        routeout->set_reverse(routeback);
        routeback->set_reverse(routeout);

        //print_route(*routeout);
        paths->push_back(routeout);
        check_non_null(routeout);
        return paths;
    }
}

void HierarchicalTopology::count_queue(Queue *queue) {
    if (_link_usage.find(queue) == _link_usage.end()) {
        _link_usage[queue] = 0;
    }

    _link_usage[queue] = _link_usage[queue] + 1;
}

int HierarchicalTopology::find_lp_switch(Queue *queue) {
    //first check worker_tor
    for (int i = 0; i < K * (K - 1); i++)
        for (int j = 0; j < K; j++)
            if (queues_worker_tor[i][j] == queue)
                return j;
    count_queue(queue);
    return -1;
}

int HierarchicalTopology::find_up_switch(Queue *queue) {
    count_queue(queue);
    //first check core_tor
    for (int i = 0; i < 1; i++)
        for (int j = 0; j < K; j++)
            if (queues_core_tor[i][j] == queue)
                return j;
    return -1;
}

int HierarchicalTopology::find_core_switch(Queue *queue) {
    count_queue(queue);
    //first check tor_core
    for (int i = 0; i < K; i++)
        for (int j = 0; j < 1; j++)
            if (queues_tor_core[i][j] == queue)
                return j;

    return -1;
}

int HierarchicalTopology::find_destination(Queue *queue) {
    //first check tor_worker
    for (int i = 0; i < K; i++)
        for (int j = 0; j < K * (K - 1); j++)
            if (queues_tor_worker[i][j] == queue)
                return j;

    return -1;
}

void HierarchicalTopology::print_path(std::ofstream &paths, int src, const Route *route) {
    paths << "SRC_" << src << " ";

    if (route->size() / 2 == 2) {
        paths << "LS_" << find_lp_switch((Queue *) route->at(1)) << " ";
        paths << "DST_" << find_destination((Queue *) route->at(3)) << " ";
    } else if (route->size() / 2 == 4) {
        paths << "LS_" << find_lp_switch((Queue *) route->at(1)) << " ";
        paths << "US_" << find_up_switch((Queue *) route->at(3)) << " ";
        paths << "LS_" << find_lp_switch((Queue *) route->at(5)) << " ";
        paths << "DST_" << find_destination((Queue *) route->at(7)) << " ";
    } else if (route->size() / 2 == 6) {
        paths << "LS_" << find_lp_switch((Queue *) route->at(1)) << " ";
        paths << "US_" << find_up_switch((Queue *) route->at(3)) << " ";
        paths << "CS_" << find_core_switch((Queue *) route->at(5)) << " ";
        paths << "US_" << find_up_switch((Queue *) route->at(7)) << " ";
        paths << "LS_" << find_lp_switch((Queue *) route->at(9)) << " ";
        paths << "DST_" << find_destination((Queue *) route->at(11)) << " ";
    } else {
        paths << "Wrong hop count " << toa(route->size() / 2);
    }

    paths << endl;
}

void HierarchicalTopology::set_switch_num_updates(
        unsigned int job_id, map<unsigned int, unsigned int> run_config) {
    std::set<unsigned> involved_tors{};
    unordered_map<unsigned, bool> cleaner_worker_set{};
    for (const auto &pair : run_config) {
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
        for (const auto &p: map) {
            myprintf("ToR %d Jid %d num_updates %d\n", tor_id, job_id, map[job_id]);
        }

        auto str = string_format("ToR %d Jid %d downward: ", tor_id, job_id);
        for (const auto &p: map_ids[job_id]) {
            str += string_format("%d ", p);
        }
        str += "\n";
        myprintf(str);
    }
    if (involved_tors.size() > 1) {
        // need to involve the core switch
        core_switch->top_level_for_job[job_id] = true;
        bool cleaner_set = false;
        std::string tors;
        for (auto& tor_id: involved_tors) {
            tors += ","+std::to_string(tor_id);
            if (!cleaner_set){
                cleaner_set = true;
                tor_switches[tor_id]->clean_upper_level_switch_for_job[job_id] = true;
            }
            tor_switches[tor_id]->top_level_for_job[job_id] = false;
        }
        myprintf("Job %u spans across multiple ToR switches: %s\n", job_id, tors.substr(1).c_str());
        core_switch->num_updates_for_job[job_id] = involved_tors.size();
        core_switch->downward_ids_for_job[job_id].merge(involved_tors);
        myprintf("core Jid %d num_updates %lu\n", job_id, core_switch->num_updates_for_job[job_id]);
        auto str = string_format("core Jid %d downward: ", job_id);
        for (const auto &p: core_switch->downward_ids_for_job[job_id]) {
            str += string_format("%d ", p);
        }
        str += "\n";
        myprintf(str);
    } else {
        core_switch->top_level_for_job[job_id] = false;
        std::string tor;
        for (auto& tor_id: involved_tors) {
            tor+=std::to_string(tor_id);
            tor_switches[tor_id]->top_level_for_job[job_id] = true;
        }
        myprintf("Job %u spans within ToR switch %s\n", job_id, tor.c_str());
    }
}

const Route *HierarchicalTopology::get_switch_single_hop_route(unsigned src, unsigned layer,
                                                               unsigned dest, bool upward) {
    // valid layer: 0, 1
    assert(layer == 1 || layer == 0);
//    auto route_out = new Route();
    if (layer == 1) { // from core to ToRs
        src = 0;
        auto key = "s"+std::to_string(src)+"d"+std::to_string(dest)+"l1u0";
        if (routes.contains(key)) {
            return routes[key];
        }
        auto route_out = new Route();
        route_out->push_back(queues_core_tor[src][dest]);
        route_out->push_back(pipes_core_tor[src][dest]);
        route_out->push_back(tor_switches[dest]);
        check_non_null(route_out);
        routes[key] = route_out;
        return route_out;
    } else if (upward) { // from ToR to core
        dest = 0;
        auto key = "s"+std::to_string(src)+"d"+std::to_string(dest)+"l0u1";
        if (routes.contains(key)) {
            return routes[key];
        }
        auto route_out = new Route();
        route_out->push_back(queues_tor_core[src][dest]);
        route_out->push_back(pipes_tor_core[src][dest]);
        route_out->push_back(core_switch.get());
        check_non_null(route_out);
        routes[key] = route_out;
        return route_out;
    } else { // from ToR to workers
        auto key = "s"+std::to_string(src)+"d"+std::to_string(dest)+"l0u0";
        if (routes.contains(key)) {
            return routes[key];
        }
        auto route_out = new Route();
        route_out->push_back(queues_tor_worker[src][dest]);
        route_out->push_back(pipes_tor_worker[src][dest]);
        route_out->push_back(_workers[dest]);
        check_non_null(route_out);
        routes[key] = route_out;
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
            delete pipes_core_tor[j][k];
        }
    }

    for (int j = 0; j < K; j++) {
        for (int k = 0; k < 1; k++) {
            delete queues_tor_core[j][k];
            delete pipes_tor_core[j][k];
        }
    }

    for (int j = 0; j < K; j++) {
        for (int k = 0; k < K * (K - 1); k++) {
            delete queues_tor_worker[j][k];
            delete pipes_tor_worker[j][k];
            delete queues_worker_tor[k][j];
            delete pipes_worker_tor[k][j];
        }
    }

    for (const auto &pair: routes) {
        delete pair.second;
    }
    routes.clear();
//    delete logfile;
//    delete eventlist;
}

std::deque<uint64_t> HierarchicalTopology::bssi(std::unordered_map<Tensor *, double> weights) {
    // coflow (per job) -> weight
    std::unordered_map<unsigned, std::unordered_map<unsigned, unsigned>> data_port_coflow; // port (per worker), coflow -> data
    std::vector<unsigned> data_port(_no_of_nodes);
    std::deque<uint64_t> result{};
    auto iters = weights.size();
    for (unsigned i = 0; i < iters; ++i) {
        if (weights.size() == 1) {
            for (auto &pair: weights) {
                result.push_front(pair.first->key);
            }
            break;
        }
        // Find the most bottlenecked port
        unsigned bottlenecked; // wid
        unsigned current_max = 0;
        for (auto &pair : weights) {
            auto &tensor = pair.first;
            for (auto wid : tensor->job->wids_allocated) {
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

        // Select weighted largest job to schedule last
        Tensor *weighted_largest;
        auto current_min = DBL_MAX;
        double min_weight;
        for (auto &pair : weights) {
            auto weight = pair.second / data_port_coflow[bottlenecked][pair.first->job->id];
            if (weight <= current_min) {
                current_min = weight;
                weighted_largest = pair.first;
                min_weight = pair.second;
            }
        }
        result.push_front(weighted_largest->key);

        // Scale the weights
        for (auto &pair : weights) {
            if (pair.first->job->id != weighted_largest->job->id) {
                pair.second -= (min_weight * data_port_coflow[bottlenecked][pair.first->job->id] /
                                data_port_coflow[bottlenecked][weighted_largest->job->id]);
            }
        }
        weights.erase(weighted_largest);
    }
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

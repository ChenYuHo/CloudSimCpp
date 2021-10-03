//#include "hierarchical_topology.h"
//#include <vector>
//#include <iostream>
//#include "main.h"
//#include "queue.h"
//#include "worker.h"
//
////extern uint32_t RTT;
//
//template <typename T>
//string toa(T n) {
//    stringstream s;
//    s << n;
//    return s.str();
//}
//
//HierarchicalTopology::HierarchicalTopology(Cluster *c, int no_of_nodes,
//                                           mem_b queuesize, Logfile *lg,
//                                           EventList *ev)
//                                           : cluster(c){
//    _queuesize = queuesize;
//    logfile = lg;
//    eventlist = ev;
//    set_params(no_of_nodes);
//    init_network();
//}
//
//void HierarchicalTopology::set_params(int no_of_nodes) {
//    cout << "Set params " << no_of_nodes << endl;
//    _no_of_nodes = 0;
//    K = 1; // Switches with K number of ports, K-ary or K-port Fat tree
//    while (_no_of_nodes < no_of_nodes) {
//        K++;
//        _no_of_nodes = K * (K - 1);
//    }
//    if (_no_of_nodes > no_of_nodes) {
//        cerr << "Topology Error: can't have a two-layer Hierarchical Topology with " << no_of_nodes
//             << " nodes\n";
//        exit(1);
//    }
//    cout << "_no_of_nodes " << _no_of_nodes << endl;
//    cout << "K " << K << endl;
//
//    tor_switches.resize(K, nullptr);
//    _workers.resize(_no_of_nodes, nullptr);
//    pipes_tor_core.resize(K, vector<Pipe *>(1));
//    pipes_core_tor.resize(1, vector<Pipe *>(K));
//    pipes_worker_tor.resize(_no_of_nodes, vector<Pipe *>(K));
//    pipes_tor_worker.resize(K, vector<Pipe *>(_no_of_nodes));
//    queues_tor_worker.resize(K, vector<Queue *>(_no_of_nodes));
//    queues_worker_tor.resize(_no_of_nodes, vector<Queue *>(K));
//    queues_tor_core.resize(K, vector<Queue *>(1));
//    queues_core_tor.resize(1, vector<Queue *>(K));
//}
//
//Queue *HierarchicalTopology::alloc_queue(QueueLogger *queueLogger, uint64_t speed = HOST_NIC) const {
//    return new RandomQueue(speedFromMbps(speed),
//                           memFromPkt(SWITCH_BUFFER + RANDOM_BUFFER),
//                           *eventlist, queueLogger, memFromPkt(RANDOM_BUFFER));
//}
//
//void HierarchicalTopology::init_network() {
//    for (int j = 0; j < 1; j++)
//        for (int k = 0; k < K; k++) {
//            queues_core_tor[j][k] = nullptr;
//            pipes_core_tor[j][k] = nullptr;
//        }
//
//    for (int j = 0; j < K; j++)
//        for (int k = 0; k < 1; k++) {
//            queues_tor_core[j][k] = nullptr;
//            pipes_tor_core[j][k] = nullptr;
//        }
//
//    for (int j = 0; j < K; j++)
//        for (int k = 0; k < K * (K - 1); k++) {
//            queues_tor_worker[j][k] = nullptr;
//            pipes_tor_worker[j][k] = nullptr;
//            queues_worker_tor[k][j] = nullptr;
//            pipes_worker_tor[k][j] = nullptr;
//        }
//
//    // instantiate workers and switches
//    core_switch = make_shared<Switch>(*eventlist, cluster);
//    for (int j = 0; j < K; j++) {
//        tor_switches[j] = make_shared<Switch>(*eventlist, cluster);
//        for (int k = 0; k < K-1; k++) {
//            _workers[k+j*(K-1)] = make_shared<Worker>(*eventlist, cluster, tor_switches[j]);
//        }
//    }
//
//    // links from ToR switch to worker
//    for (int j = 0; j < K; j++) {
//        for (int l = 0; l < K - 1; l++) {
//            int k = j * (K - 1) + l;
//            // Downlink
//            queues_tor_worker[j][k] = alloc_queue(nullptr);
//            queues_tor_worker[j][k]->setName("ToR" + toa(j) + "->SERVER" + toa(k));
//            pipes_tor_worker[j][k] = new Pipe(timeFromUs(RTT), *eventlist);
//            pipes_tor_worker[j][k]->setName("Pipe-ToR" + toa(j) + "->SERVER" + toa(k));
//
//            // Uplink
//            queues_worker_tor[k][j] = alloc_queue(nullptr);
//            queues_worker_tor[k][j]->setName("SERVER" + toa(k) + "->ToR" + toa(j));
//            pipes_worker_tor[k][j] = new Pipe(timeFromUs(RTT), *eventlist);
//            pipes_worker_tor[k][j]->setName("Pipe-SERVER" + toa(k) + "->ToR" + toa(j));
//        }
//    }
//
//    // ToR to core!
//    for (int j = 0; j < K; j++) {
//        int k = 0;
//        // Downlink
//        queues_tor_core[j][k] = alloc_queue(nullptr);
//        queues_tor_core[j][k]->setName("ToR" + toa(j) + "->Core" + toa(k));
//        pipes_tor_core[j][k] = new Pipe(timeFromUs(RTT), *eventlist);
//        pipes_tor_core[j][k]->setName("Pipe-ToR" + toa(j) + "->Core" + toa(k));
//
//        // Uplink
//        queues_core_tor[k][j] = alloc_queue(nullptr);
//        queues_core_tor[k][j]->setName("Core" + toa(k) + "->ToR" + toa(j));
//        pipes_core_tor[k][j] = new Pipe(timeFromUs(RTT), *eventlist);
//        pipes_core_tor[k][j]->setName("Pipe-Core" + toa(k) + "->ToR" + toa(j));
//    }
//
//}
//
//void check_non_null(Route *rt) {
//    int fail = 0;
//    for (unsigned int i = 1; i < rt->size() - 1; i += 2)
//        if (rt->at(i) == nullptr) {
//            fail = 1;
//            break;
//        }
//
//    if (fail) {
//        //    cout <<"Null queue in route"<<endl;
//        for (unsigned int i = 1; i < rt->size() - 1; i += 2)
//            printf("%p ", rt->at(i));
//
//        cout << endl;
//        assert(0);
//    }
//}
//
//const Route *HierarchicalTopology::get_worker_to_tor_path(int src, int dest) {
//    route_t *route_out;
//
//    if (HOST_ToR_SWITCH(src) == dest) {
//        route_out = new Route();
//        route_out->push_back(queues_worker_tor[src][dest]);
//        route_out->push_back(pipes_worker_tor[src][dest]);
////        route_out->push_back(tor_switches[dest].get());
//        check_non_null(route_out);
//    }
//    printf("%d, %d, %p\n", HOST_ToR_SWITCH(src), dest, tor_switches[dest].get());
//    return route_out;
//}
//
//const Route *HierarchicalTopology::get_tor_to_worker_path(int src, int dest) {
//    route_t *route_out;
//    if (HOST_ToR_SWITCH(dest) == src) {
//        route_out = new Route();
//        route_out->push_back(queues_tor_worker[src][dest]);
//        route_out->push_back(pipes_tor_worker[src][dest]);
////        route_out->push_back(_workers[dest].get());
//        check_non_null(route_out);
//    }
//    return route_out;
//}
//
//vector<const Route *> *HierarchicalTopology::get_paths(int src, int dest) {
//    auto *paths = new vector<const Route *>();
//
//    route_t *routeout, *routeback;
//    //QueueLoggerSimple *simplequeuelogger = new QueueLoggerSimple();
//    //QueueLoggerSimple *simplequeuelogger = 0;
//    //logfile->addLogger(*simplequeuelogger);
//    //Queue* pqueue = new Queue(speedFromMbps((uint64_t)HOST_NIC), memFromPkt(FEEDER_BUFFER), *eventlist, simplequeuelogger);
//    //pqueue->setName("PQueue_" + toa(src) + "_" + toa(dest));
//    //logfile->writeName(*pqueue);
//    if (HOST_ToR_SWITCH(src) == HOST_ToR_SWITCH(dest)) {
//
//        // forward path
//        routeout = new Route();
//        //routeout->push_back(pqueue);
//        routeout->push_back(queues_worker_tor[src][HOST_ToR_SWITCH(src)]);
//        routeout->push_back(pipes_worker_tor[src][HOST_ToR_SWITCH(src)]);
//
////        routeout->push_back(tor_switches[HOST_ToR_SWITCH(src)]);
//
//        routeout->push_back(queues_tor_worker[HOST_ToR_SWITCH(dest)][dest]);
//        routeout->push_back(pipes_tor_worker[HOST_ToR_SWITCH(dest)][dest]);
//
//        // reverse path for RTS packets
//        routeback = new Route();
//        routeback->push_back(queues_worker_tor[dest][HOST_ToR_SWITCH(dest)]);
//        routeback->push_back(pipes_worker_tor[dest][HOST_ToR_SWITCH(dest)]);
//
////        routeback->push_back(tor_switches[HOST_ToR_SWITCH(dest)]);
//
//        routeback->push_back(queues_tor_worker[HOST_ToR_SWITCH(src)][src]);
//        routeback->push_back(pipes_tor_worker[HOST_ToR_SWITCH(src)][src]);
//
//        routeout->set_reverse(routeback);
//        routeback->set_reverse(routeout);
////        cout<<"print route\n";
////        print_route(*routeout);
//        paths->push_back(routeout);
//
//        check_non_null(routeout);
//        return paths;
//    } else {
//        int core = 0;
//        routeout = new Route();
//        //routeout->push_back(pqueue);
//        routeout->push_back(queues_worker_tor[src][HOST_ToR_SWITCH(src)]);
//        routeout->push_back(pipes_worker_tor[src][HOST_ToR_SWITCH(src)]);
//
//        routeout->push_back(queues_tor_core[HOST_ToR_SWITCH(src)][core]);
//        routeout->push_back(pipes_tor_core[HOST_ToR_SWITCH(src)][core]);
//
//        //now take the only link down to the destination worker!
//
////        int HOST_ToR_SWITCH(dest) = HOST_POD(dest) * K / 2 + 2 * core / K;
//        //printf("K %d HOST_POD(%d) %d core %d HOST_ToR_SWITCH(dest) %d\n",K,dest,HOST_POD(dest),core, HOST_ToR_SWITCH(dest));
//
//        routeout->push_back(queues_core_tor[core][HOST_ToR_SWITCH(dest)]);
//        routeout->push_back(pipes_core_tor[core][HOST_ToR_SWITCH(dest)]);
//
//        routeout->push_back(queues_tor_worker[HOST_ToR_SWITCH(dest)][dest]);
//        routeout->push_back(pipes_tor_worker[HOST_ToR_SWITCH(dest)][dest]);
//
//        // reverse path for RTS packets
//        routeback = new Route();
//
//        routeback->push_back(queues_worker_tor[dest][HOST_ToR_SWITCH(dest)]);
//        routeback->push_back(pipes_worker_tor[dest][HOST_ToR_SWITCH(dest)]);
//
//        routeback->push_back(queues_tor_core[HOST_ToR_SWITCH(dest)][core]);
//        routeback->push_back(pipes_tor_core[HOST_ToR_SWITCH(dest)][core]);
//
//        //now take the only link back down to the src worker!
//
//        routeback->push_back(queues_core_tor[core][HOST_ToR_SWITCH(src)]);
//        routeback->push_back(pipes_core_tor[core][HOST_ToR_SWITCH(src)]);
//        routeback->push_back(queues_tor_worker[HOST_ToR_SWITCH(src)][src]);
//        routeback->push_back(pipes_tor_worker[HOST_ToR_SWITCH(src)][src]);
//
//
//        routeout->set_reverse(routeback);
//        routeback->set_reverse(routeout);
//
//        //print_route(*routeout);
//        paths->push_back(routeout);
//        check_non_null(routeout);
//        return paths;
//    }
//}
//
//void HierarchicalTopology::count_queue(Queue *queue) {
//    if (_link_usage.find(queue) == _link_usage.end()) {
//        _link_usage[queue] = 0;
//    }
//
//    _link_usage[queue] = _link_usage[queue] + 1;
//}
//
//int HierarchicalTopology::find_lp_switch(Queue *queue) {
//    //first check worker_tor
//    for (int i = 0; i < K * (K - 1); i++)
//        for (int j = 0; j < K; j++)
//            if (queues_worker_tor[i][j] == queue)
//                return j;
//    count_queue(queue);
//    return -1;
//}
//
//int HierarchicalTopology::find_up_switch(Queue *queue) {
//    count_queue(queue);
//    //first check core_tor
//    for (int i = 0; i < 1; i++)
//        for (int j = 0; j < K; j++)
//            if (queues_core_tor[i][j] == queue)
//                return j;
//    return -1;
//}
//
//int HierarchicalTopology::find_core_switch(Queue *queue) {
//    count_queue(queue);
//    //first check tor_core
//    for (int i = 0; i < K; i++)
//        for (int j = 0; j < 1; j++)
//            if (queues_tor_core[i][j] == queue)
//                return j;
//
//    return -1;
//}
//
//int HierarchicalTopology::find_destination(Queue *queue) {
//    //first check tor_worker
//    for (int i = 0; i < K; i++)
//        for (int j = 0; j < K * (K - 1); j++)
//            if (queues_tor_worker[i][j] == queue)
//                return j;
//
//    return -1;
//}
//
//void HierarchicalTopology::print_path(std::ofstream &paths, int src, const Route *route) {
//    paths << "SRC_" << src << " ";
//
//    if (route->size() / 2 == 2) {
//        paths << "LS_" << find_lp_switch((Queue *) route->at(1)) << " ";
//        paths << "DST_" << find_destination((Queue *) route->at(3)) << " ";
//    } else if (route->size() / 2 == 4) {
//        paths << "LS_" << find_lp_switch((Queue *) route->at(1)) << " ";
//        paths << "US_" << find_up_switch((Queue *) route->at(3)) << " ";
//        paths << "LS_" << find_lp_switch((Queue *) route->at(5)) << " ";
//        paths << "DST_" << find_destination((Queue *) route->at(7)) << " ";
//    } else if (route->size() / 2 == 6) {
//        paths << "LS_" << find_lp_switch((Queue *) route->at(1)) << " ";
//        paths << "US_" << find_up_switch((Queue *) route->at(3)) << " ";
//        paths << "CS_" << find_core_switch((Queue *) route->at(5)) << " ";
//        paths << "US_" << find_up_switch((Queue *) route->at(7)) << " ";
//        paths << "LS_" << find_lp_switch((Queue *) route->at(9)) << " ";
//        paths << "DST_" << find_destination((Queue *) route->at(11)) << " ";
//    } else {
//        paths << "Wrong hop count " << toa(route->size() / 2);
//    }
//
//    paths << endl;
//}
//
////void HierarchicalTopology::register_switch(Switch *s) {
////    printf("register %d %p\n", s->id(), s);
//////    cout<<s->id<<s;
////    tor_switches[s->id()] = s;
////}
////
////void HierarchicalTopology::register_core_switch(Switch *s) {
////    core_switch[s->id()] = s;
////}
////
////void HierarchicalTopology::register_worker(Worker *w) {
////    _workers[w->id()] = w;
////}

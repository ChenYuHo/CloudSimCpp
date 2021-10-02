#include "cluster.h"
#include "topology/hierarchical_topology.h"

//vector<const Route *> *Cluster::get_paths(int src, int dest) {
//    return nullptr;
//}
//
//int Cluster::no_of_nodes() const {
//    return Topology::no_of_nodes();
//}
//
//vector<int> *Cluster::get_neighbours(int src) {
//    return nullptr;
//}

void Cluster::doNextEvent() {
//    if (!all_jobs_started) {
//        printf("[%llu] scheduler invoked\n", eventlist().now());
////        eventlist().sourceIsPendingRel(*this, 1000000000);
//        Job *job = _scheduling->choose_job_to_execute_in(this);
//        if (job != nullptr) {
//            printf("[%llu]\tjob %d which requires %d gpus is chosen\n", eventlist().now(), job->id(), job->gpus_required);
//            auto run_config = _placement->place_job_in(this, job);
////            if (run_config.empty()) {
////                printf("[%llu]\tplacement failed for task %d requiring %d GPUs\n", eventlist().now(), job->id, job->gpus_required);
////            } else {
////                printf("[%llu]\tjob %d placement: ", eventlist().now(), job->id);
////                job->num_workers_allocated = run_config.size();
////                for (const auto &pair: run_config) {
////                    printf("mid %d -> %d gpus_required ", pair.first, pair.second);
////                    cluster.machine_map[pair.first]->execute_job(sim, job, pair.second, cs);
////                }
////                printf("\n");
////            }
////            co_await sim.timeout(0);
////            continue; // There could be multiple jobs with the same submission timestamp
//        }
////        co_await sim.timeout(1e10);
//    }
//    all_jobs_started = true;

}

void Cluster::init_topo() {
    _topo = (Topology *) new HierarchicalTopology(this,
                                                  12,
                                                  memFromPkt(8),
                                                  nullptr,
                                                  &_eventlist);
    _workers = _topo->workers();
    _switches = _topo->switches();

}

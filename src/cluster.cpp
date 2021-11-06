#include <memory>
#include <vector>
#include "cluster.h"
#include "job.h"
#include "worker.h"
#include "switch.h"
#include "topology/hierarchical_topology.h"

class Worker;
class Switch;

void Cluster::setup(config conf) {
//    for (int i = 0; i < conf.n_tor; i++) {
//        auto s = std::make_shared<Switch>(eventlist());
//        for (int j = 0; j < conf.m_per_tor; j++) {
//            auto m = std::make_shared<Worker>(eventlist(), this, s);
//            workers.push_back(m);
//            worker_map[m->id] = workers.back();
//            s->machines.push_back(m);
//        }
//        switches.push_back(s);
//    }
}


void Cluster::check_if_all_jobs_started() {
    if (!all_jobs_submitted) return;
    for (std::shared_ptr<Job> j:jobs) {
        if (j->start_time == std::numeric_limits<uint64_t>::max()) return; // at least one job not started yet
    }
    all_jobs_started = true;
}


void Cluster::check_if_all_jobs_finished() {
    if (!all_jobs_started) return;
    for (std::shared_ptr<Job> j:jobs) {
        if (j->finish_time == std::numeric_limits<uint64_t>::max()) return; // at least one job not finished yet
    }
    all_jobs_finished = true;
}

void Cluster::init_topo() {
    _topo = (Topology *) new HierarchicalTopology(this,
                                                  SWITCH_PORTS,
                                                  SWITCH_BUFFER,
                                                  nullptr,
                                                  &_eventlist);
    workers = _topo->workers();
    for (const auto& w : workers) worker_map[w->id] = w;
//    machine_map[m->id] = machines.back();
    switches = _topo->switches();
}

void Cluster::doNextEvent() {}

Cluster::~Cluster() {
    delete (HierarchicalTopology*)_topo;
}

//#include "cluster.h"
//#include "topology/hierarchical_topology.h"
//
////vector<const Route *> *Cluster::get_paths(int src, int dest) {
////    return nullptr;
////}
////
////int Cluster::no_of_nodes() const {
////    return Topology::no_of_nodes();
////}
////
////vector<int> *Cluster::get_neighbours(int src) {
////    return nullptr;
////}
//
//void Cluster::doNextEvent() {
////    if (!all_jobs_started) {
////        myprintf("[%llu] scheduler invoked\n", eventlist().now());
//////        eventlist().sourceIsPendingRel(*this, 1000000000);
////        Job *job = _scheduling->choose_job_to_execute_in(this);
////        if (job != nullptr) {
////            myprintf("[%llu]\tjob %d which requires %d gpus is chosen\n", eventlist().now(), job->id(), job->gpus_required);
////            auto run_config = _placement->place_job_in(this, job);
//////            if (run_config.empty()) {
//////                myprintf("[%llu]\tplacement failed for task %d requiring %d GPUs\n", eventlist().now(), job->id, job->gpus_required);
//////            } else {
//////                myprintf("[%llu]\tjob %d placement: ", eventlist().now(), job->id);
//////                job->num_workers_allocated = run_config.size();
//////                for (const auto &pair: run_config) {
//////                    myprintf("mid %d -> %d gpus_required ", pair.first, pair.second);
//////                    cluster.machine_map[pair.first]->execute_job(sim, job, pair.second, cs);
//////                }
//////                myprintf("\n");
//////            }
//////            co_await sim.timeout(0);
//////            continue; // There could be multiple jobs with the same submission timestamp
////        }
//////        co_await sim.timeout(1e10);
////    }
////    all_jobs_started = true;
//
//}
//
//void Cluster::init_topo() {
//    _topo = (Topology *) new HierarchicalTopology(this,
//                                                  12,
//                                                  memFromPkt(8),
//                                                  nullptr,
//                                                  &_eventlist);
//    _workers = _topo->workers();
////    _switches = _topo->switches();
//
//}

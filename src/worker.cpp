#include <vector>
#include <memory>
#include <sstream>
#include <ranges>
#include "common.h"
#include "worker.h"
#include "job.h"
#include "switch.h"
#include "cluster.h"
#include "topology/hierarchical_topology.h"
#include "packet.h"


//simcpp20::event<SIM_UNIT>
//Worker::allreduce(simcpp20::simulation<SIM_UNIT> &sim, uint64_t grad_size, std::string tensor_id,
//                  std::shared_ptr<Job> job) {
//    auto start = sim.now();
//    printf("[ALLREDUCE] jid %d mid %d tid %s size %llu start %llu\n",
//           job->id, id, tensor_id.c_str(), grad_size, start);
////    if (job->id == JID)
////        printf("[1,%d]<stdout>:SYNTHETIC COLLECTIVE START %llu %llu\n", id, uint64_t(start * 1e9), grad_size);
////    co_await lock.at(id).at(job->id)->request(); // allreduce goes one by one (non-preemptive)
//
//    const static uint64_t pkt_size = 1024 * 1024; // number of elements
//    printf("[%llu]\tWorker %d Job %d invoke allreduce on %llu\n", sim.now(), id, job->id, grad_size);
//    std::vector<simcpp20::event<SIM_UNIT>> pkt_events;
//    unsigned i = 0;
//    uint64_t c = 0;
//    for (; i < grad_size / pkt_size; ++i) {
//        std::stringstream s;
//        std::string all_ids;
//        s << job->id << "," << tensor_id << "," << i;
//        s >> all_ids; // job_id, tensor_id, pkt_id
//        printf("[%llu]\tWorker %d Job %d tensor %d send packet %d\n", sim.now(), id, job->id, grad_size, i);
//        pkt_events.push_back(tor->send_receive(sim, pkt_size, all_ids, job->num_workers_allocated, id));
//        co_await sim.timeout(0);
//        c += pkt_size;
//    }
//    if (grad_size % pkt_size) {
//        std::stringstream s;
//        std::string all_ids;
//        s << job->id << "," << tensor_id << "," << i;
//        s >> all_ids; // job_id, tensor_id, pkt_id
//        printf("[%llu]\tWorker %d Job %d tensor %d send packet %d\n", sim.now(), id, job->id, grad_size, i);
//        pkt_events.push_back(tor->send_receive(sim, grad_size % pkt_size, all_ids, job->num_workers_allocated, id));
//        c += grad_size % pkt_size;
//    }
//    assert(c == grad_size);
//    co_await sim.all_of(pkt_events);
////    printf("[%.3f]\tWorker %d done allreduce for Job %d on %d\n", sim.now(), id, job->id, grad_size);
////    auto duration = sim.now() - start;
////    if (job->id == JID) {
////        printf("[1,%d]<stdout>:SYNTHETIC COLLECTIVE PROFILE %llu %llu 1 %llu 0\n", id, uint64_t(start * 1e9),
////               uint64_t(duration * 1e9), grad_size);
////    }
////    lock.at(id).at(job->id)->release();
//}

simcpp20::event<SIM_UNIT>
Worker::execute_job(simcpp20::simulation<SIM_UNIT> &sim, std::shared_ptr<Job> job, const unsigned gpus_required,
                    CollectiveScheduler *cs) {
    jobs.push_back(job);
//    printf("[%llu]\tmachine %d free gpu %d, job %d asking %d\n", sim.now(), id, gpu, job->id, gpus_required);
    gpu -= gpus_required;
    assert(gpu >= 0);
    job->start_time = sim.now();
    cluster->check_if_all_jobs_started();

    std::vector<std::shared_ptr<Tensor>> tensors;
    unsigned tensor_id = 0;
    for (uint64_t i = 0; i < job->model.size(); ++i) {
        tensors.push_back(
                std::make_shared<Tensor>(Tensor(i, job->model[i], this, job, sim)));
        locks.emplace(job->id * 10000 + i,
                      make_shared<resource<SIM_UNIT>>(sim, 1));
    }
//    for (auto grad_size: job->model) {
//        tensors.push_back(
//                std::make_shared<Tensor>(Tensor(grad_size, this, job, sim)));
//    }

//    auto &jid_allreduce_lock = allreduce_lock[id];
//    jid_allreduce_lock.erase(job->id);
//    jid_allreduce_lock.emplace(job->id, std::make_shared<resource<SIM_UNIT>>(sim, 1));
//    printf("NITER %d\n", job->n_iter);

    for (unsigned iter = 0; iter < job->n_iter; ++iter) {
//        printf("Worker %d starting iter %d for job %d\n", id, iter, job->id);
//        printf("[1,%d]<stdout>:SYNTHETIC ITERATION START PROFILE %llu\n", id, sim.now());

        for (auto &tensor: tensors) {
            printf("%d waiting LOCKF %p %llu %d\n", id, tensor.get(), tensor->tensor_id, tensor->iter);
            auto &lock = locks[job->id * 10000 + tensor->tensor_id];
            co_await lock->request(); // block if last step's allreduce is not completed yet
            printf("%d LOCKF %llu %d\n", id, tensor->tensor_id, tensor->iter);
            tensor->allreduced_size = 0;
            auto fptime = forward_pass_time(tensor->size);
            auto forward_begin = sim.now();
            co_await sim.timeout(fptime);
//            if (PRINT) {
            printf("[forward] iter %d jid %d mid %d tid %llu size %llu start %llu duration %llu end %llu\n", iter,
                   job->id, id, tensor->tensor_id, tensor->size, forward_begin, fptime, sim.now());
//            }
            lock->release();
//            printf("%d ULOCKF %llu %d\n", id, tensor->tensor_id, tensor->iter);
        }
        //    printf("[%llu]\tWorker %d job %d forward pass all done\n", sim.now(), id, job->id);

        std::ranges::reverse_view reversed{tensors};
//        unsigned tensor_id = 0;
        for (auto &tensor: reversed) {
            auto bptime = backward_pass_time(tensor->size);
            auto backward_begin = sim.now();
            co_await sim.timeout(bptime);
//            if (PRINT) {
            printf("[backward] iter %d jid %d mid %d tid %d size %d start %llu duration %llu end %llu\n", iter,
                   job->id, id, tensor->tensor_id, tensor->size, backward_begin, bptime, sim.now());
//            }
            co_await locks[job->id * 10000 +
                           tensor->tensor_id]->request(); // release after allreduce is done so next fp can proceed
//            printf("%d LOCKB %llu %d\n", id, tensor->tensor_id, tensor->iter);
            //            printf("got lock for iter %d jid %d mid %d tid %d size %d\n", tensor->iter,
//                   job->id, id, tensor->tensor_id, tensor->size);

            tensor->iter++;
            //modified temporarily
            //
//            allreduce(sim, tensor); // nonblocking


            // original
            if (cs != nullptr) cs->enqueue(tensor);





//            auto chunk_size = CHUNK_SIZE;
//            if (chunk_size) {
//                unsigned chunk_id = 0;
//                for (unsigned i = 0; i < tensor.size / chunk_size; ++i) {
//                    cs->enqueue(chunk_size, this, tensor_id, chunk_id++, job);
//                }
//                if (tensor.size % chunk_size) {
//                    cs->enqueue(tensor.size % chunk_size, this, tensor_id, chunk_id++, job);
//                }
//                tensor_id++;
//            } else {
//                cs->enqueue(tensor.size, this, tensor_id++, 0, job);
//            }
//            co_await sim.timeout(0);
//            allreduce_events.push_back(allreduce(sim, grad_size, tensor_id++, job)); // allreduce doesn't block
        }
//        co_await sim.all_of(allreduce_events);
//        printf("[%llu]\tmachine %d backward loop over\n", sim.now(), id);
//        if(job->id==JID) printf("[1,%d]<stdout>:SYNTHETIC ITERATION END PROFILE %llu\n", id, sim.now());
    }
    for (auto &tensor: tensors) {
        printf("waiting start for iter %d jid %d mid %d tid %llu size %llu", tensor->iter,
               job->id, id, tensor->tensor_id, tensor->size);
        auto &lock = locks[job->id * 10000 + tensor->tensor_id];
        co_await lock->request(); // wait until final allreduces are done
        lock->release();
//        locks.erase(job->id * 10000 + tensor->tensor_id);
//        printf("%d LOCKFINAL %llu %d\n", id, tensor->tensor_id, tensor->iter);
        printf("waiting done for iter %d jid %d mid %d tid %llu size %llu\n", tensor->iter,
               job->id, id, tensor->tensor_id, tensor->size);
    }
    job->finish_time = sim.now();
    gpu += gpus_required;
    printf("[%lu]\tmachine %d finishes job %d and has %d gpu now\n", sim.now(), id, job->id, gpu);
    assert(gpu <= gpu_capacity);
    cluster->check_if_all_jobs_finished();
}

void Worker::doNextEvent() {

}

void Worker::receivePacket(Packet &pkt) {
    auto p = (SwitchMLPacket *) &pkt;
    printf("[%lu] worker %d got aggregated pkt JID %d slot %d offset %d\n",
           eventlist().now(), id, p->job_id, p->slot, p->offset);
    unsigned pkt_idx = p->offset / NUM_UPDATES;
    auto &set = received_pkts[p->tensor->tensor_id];
    if (set.contains(pkt_idx)) {
        // duplicate
        printf("already received %d/%d pkt, discarding\n", pkt_idx, p->tensor->num_pkts_expected);
        return;
    }
    set.insert(pkt_idx);
    // cancel timer
    if (set.size() == p->tensor->num_pkts_expected) {
        printf("DONE %llu %d %d %d\n", p->tensor->tensor_id, p->tensor->iter, p->ver, p->slot);
//        p->tensor->received_pkts.clear(); // counter for switch
//        for (auto &s : cluster->_topo->switches()) {
//            s->received_pkts[p->ver][p->slot].clear();
//            s->count[p->ver][p->slot] = 0;
//        }
        received_pkts.erase(p->tensor->tensor_id); // counter for worker
//        printf("%d RELEASE LOCKDONE %p %llu %d\n", id, p->tensor.get(), p->tensor->tensor_id, p->tensor->iter);
        locks[p->job_id * 10000 + p->tensor->tensor_id]->release();
//        printf("%d LOCKDONE %llu %d\n", id, p->tensor->tensor_id, p->tensor->iter);
        return;
    }
    auto next_start = p->offset + NUM_UPDATES * NUM_SLOTS;
    if (next_start >= p->grad_size) return;
    sendPacket(next_start, 1 - p->ver, p->slot, p->grad_size, p->tensor);
//    printf("[%llu] worker %d got aggregated pkt JID %d slot %d offset %d\n",
//           eventlist().now(), id, p->job_id, p->slot, p->offset);

}


//#include "worker.h"
//#include "cluster.h"
//#include "topology/hierarchical_topology.h"
//#include "fschuetz04/simcpp20.hpp"
//
//void Worker::doNextEvent() {
//    // schedule start of allreduce
//    allreduce(0);
//}
//
//void Worker::receivePacket(Packet &packet) {
//    printf("[%llu] worker got packet\n", eventlist().now());
//}
//
void Worker::sendPacket(unsigned start, unsigned ver,
                        unsigned slot, unsigned grad_size,
                        const shared_ptr<Tensor> &tensor) {
    auto topo = cluster->_topo;
    const Route *route = topo->get_worker_to_tor_path(id);
    auto p = SwitchMLPacket::newpkt(*route);
    p->id = id;
    p->ver = ver;
    p->slot = slot;
    p->offset = start;
    p->upward = true;
    p->grad_size = grad_size;
    p->n_workers = tensor->job->num_workers_allocated;
    p->job_id = tensor->job->id;
    p->tensor = tensor;
    p->set_ts(eventlist().now());
    p->print_info(0, eventlist().now(), 0);
    print_route(*route);
    p->sendOn();
}


//class Timer : public EventSource {
//public:
//    Timer(EventList &eventlist, const string &name) : EventSource(eventlist, "Timer") {
//
//    };
//
//    void doNextEvent() override {
//        std::cout << "test event source " << eventlist().now() << std::endl;
//    }
//};
simcpp20::event<SIM_UNIT> Worker::allreduce(simcpp20::simulation<SIM_UNIT> &sim,
                                            const shared_ptr<Tensor> &tensor) {
    auto grad_size = tensor->size;
    printf("[%llu] worker %d start allreduce for tensor size %llu iter %d\n", eventlist().now(), id, grad_size,
           tensor->iter);
    tensor->num_pkts_expected = grad_size / NUM_UPDATES;
    if (grad_size % NUM_UPDATES) tensor->num_pkts_expected += 1;
    for (unsigned slot = 0; slot < NUM_SLOTS; ++slot) {
        auto start = slot * NUM_UPDATES;
        if (start >= grad_size) break;
        sendPacket(start, 0, slot, grad_size, tensor);
    }
    auto key = tensor->job->id * 10000 + tensor->tensor_id;
    co_await locks[key]->request();
//    co_await sim.timeout(0);
    locks[key]->release();
//    tensor->lock.release();
}
//
//void Worker::execute_job(const std::shared_ptr<Job> &job, const unsigned int gpus_required) {
//    jobs.push_back(job);
//    printf("[%llu]\tworker %d executing job %d using %d gpus\n", eventlist().now(), _id, job->id(), gpus_required);
//    _gpu -= gpus_required;
//    assert(_gpu >= 0);
//
////    std::vector<std::shared_ptr<Tensor>> tensors;
////    unsigned tensor_id = 0;
////    for (auto grad_size: job->model) {
////        tensors.push_back(
////                std::make_shared<Tensor>(Tensor{grad_size, 0, this, job, tensor_id++, 0, 0, resource(sim, 1)}));
////    }
////
////    auto &jid_allreduce_lock = allreduce_lock[id];
////    jid_allreduce_lock.erase(job->id);
////    jid_allreduce_lock.emplace(job->id, std::make_shared<resource<SIM_UNIT>>(sim, 1));
////
//    for (unsigned iter = 0; iter < job->n_iter; ++iter) {
//////        if (job->id == JID) printf("[1,%d]<stdout>:SYNTHETIC ITERATION START PROFILE %llu\n", id, sim.now());
////
////        for (auto &tensor: tensors) {
////            co_await tensor->lock.request(); // block if last step's allreduce is not completed yet
////            tensor->allreduced_size = 0;
////            auto fptime = forward_pass_time(tensor->size);
////            auto forward_begin = sim.now();
////            co_await sim.timeout(fptime);
////            if (PRINT) {
////                printf("[forward] iter %d jid %d mid %d tid %d size %llu start %llu duration %llu end %llu\n", iter,
////                       job->id, id, tensor->tensor_id, tensor->size, forward_begin, fptime, sim.now());
////            }
////            tensor->lock.release();
////        }
////        //    printf("[%llu]\tWorker %d job %d forward pass all done\n", sim.now(), id, job->id);
////
////        std::ranges::reverse_view reversed{tensors};
////        unsigned tensor_id = 0;
////        for (auto &tensor: reversed) {
////            auto bptime = backward_pass_time(tensor->size);
////            auto backward_begin = sim.now();
////            co_await sim.timeout(bptime);
////            if (PRINT) {
////                printf("[backward] iter %d jid %d mid %d tid %d size %d start %llu duration %llu end %llu\n", iter,
////                       job->id, id, tensor->tensor_id, tensor->size, backward_begin, bptime, sim.now());
////            }
////            co_await tensor->lock.request(); // release after allreduce is done so next fp can proceed
////            printf("got lock for iter %d jid %d mid %d tid %d size %d\n", tensor->iter,
////                   job->id, id, tensor->tensor_id, tensor->size);
////            cs->enqueue(tensor);
//////            auto chunk_size = CHUNK_SIZE;
//////            if (chunk_size) {
//////                unsigned chunk_id = 0;
//////                for (unsigned i = 0; i < tensor.size / chunk_size; ++i) {
//////                    cs->enqueue(chunk_size, this, tensor_id, chunk_id++, job);
//////                }
//////                if (tensor.size % chunk_size) {
//////                    cs->enqueue(tensor.size % chunk_size, this, tensor_id, chunk_id++, job);
//////                }
//////                tensor_id++;
//////            } else {
//////                cs->enqueue(tensor.size, this, tensor_id++, 0, job);
//////            }
//////            co_await sim.timeout(0);
//////            allreduce_events.push_back(allreduce(sim, grad_size, tensor_id++, job)); // allreduce doesn't block
////        }
//////        co_await sim.all_of(allreduce_events);
//////        printf("[%llu]\tmachine %d backward loop over\n", sim.now(), id);
//////        if(job->id==JID) printf("[1,%d]<stdout>:SYNTHETIC ITERATION END PROFILE %llu\n", id, sim.now());
//    }
////    for (auto &tensor: tensors) {
////        printf("waiting start for iter %d jid %d mid %d tid %d size %lu\n", tensor->iter,
////               job->id, id, tensor->tensor_id, tensor->size);
////        co_await tensor->lock.request(); // wait until final allreduces are done
////        printf("waiting done for iter %d jid %d mid %d tid %d size %lu\n", tensor->iter,
////               job->id, id, tensor->tensor_id, tensor->size);
////    }
////    job->finish_time = sim.now();
////    gpu += gpus_required;
////    printf("[%llu]\tmachine %d finishes job %d and has %d gpu now\n", sim.now(), id, job->id, gpu);
////    assert(gpu <= gpu_capacity);
////    cluster->check_if_all_jobs_finished();
//
//}

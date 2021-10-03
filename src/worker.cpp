#include <vector>
#include <memory>
#include <sstream>
#include <ranges>
#include "common.h"
#include "worker.h"
#include "job.h"
#include "switch.h"
#include "cluster.h"


simcpp20::event<SIM_UNIT>
Worker::allreduce(simcpp20::simulation<SIM_UNIT> &sim, uint64_t grad_size, std::string tensor_id,
                  std::shared_ptr<Job> job) {
    auto start = sim.now();
//    printf("[ALLREDUCE] jid %d mid %d tid %s size %llu start %llu\n",
//                           job->id, id, tensor_id.c_str(), grad_size, start);
//    if (job->id == JID)
//        printf("[1,%d]<stdout>:SYNTHETIC COLLECTIVE START %llu %llu\n", id, uint64_t(start * 1e9), grad_size);
//    co_await lock.at(id).at(job->id)->request(); // allreduce goes one by one (non-preemptive)

    const static uint64_t pkt_size = 1024 * 1024; // number of elements
//    printf("[%.3f]\tWorker %d Job %d invoke allreduce on %llu\n", sim.now(), id, job->id, grad_size);
    std::vector<simcpp20::event<SIM_UNIT>> pkt_events;
    unsigned i = 0;
    uint64_t c = 0;
    for (; i < grad_size / pkt_size; ++i) {
        std::stringstream s;
        std::string all_ids;
        s << job->id << "," << tensor_id << "," << i;
        s >> all_ids; // job_id, tensor_id, pkt_id
        printf("[%llu]\tWorker %d Job %d tensor %d send packet %d\n", sim.now(), id, job->id, grad_size, i);
        pkt_events.push_back(tor->send_receive(sim, pkt_size, all_ids, job->num_workers_allocated, id));
        co_await sim.timeout(0);
        c += pkt_size;
    }
    if (grad_size % pkt_size) {
        std::stringstream s;
        std::string all_ids;
        s << job->id << "," << tensor_id << "," << i;
        s >> all_ids; // job_id, tensor_id, pkt_id
        printf("[%llu]\tWorker %d Job %d tensor %d send packet %d\n", sim.now(), id, job->id, grad_size, i);
        pkt_events.push_back(tor->send_receive(sim, grad_size % pkt_size, all_ids, job->num_workers_allocated, id));
        c += grad_size % pkt_size;
    }
    assert(c == grad_size);
    co_await sim.all_of(pkt_events);
//    printf("[%.3f]\tWorker %d done allreduce for Job %d on %d\n", sim.now(), id, job->id, grad_size);
//    auto duration = sim.now() - start;
//    if (job->id == JID) {
//        printf("[1,%d]<stdout>:SYNTHETIC COLLECTIVE PROFILE %llu %llu 1 %llu 0\n", id, uint64_t(start * 1e9),
//               uint64_t(duration * 1e9), grad_size);
//    }
//    lock.at(id).at(job->id)->release();
}

simcpp20::event<SIM_UNIT>
Worker::execute_job(simcpp20::simulation<SIM_UNIT> &sim, std::shared_ptr<Job> job, const unsigned gpus_required,
                    CollectiveScheduler *cs) {
    jobs.push_back(job);
    printf("[%llu]\tmachine %d free gpu %d, job %d asking %d\n", sim.now(), id, gpu, job->id, gpus_required);
    gpu -= gpus_required;
    assert(gpu >= 0);
    job->start_time = sim.now();
    cluster->check_if_all_jobs_started();

    std::vector<std::shared_ptr<Tensor>> tensors;
    unsigned tensor_id = 0;
    for (auto grad_size: job->model) {
        tensors.push_back(
                std::make_shared<Tensor>(Tensor{grad_size, 0, this, job, tensor_id++, 0, 0, resource(sim, 1)}));
    }

    auto &jid_allreduce_lock = allreduce_lock[id];
    jid_allreduce_lock.erase(job->id);
    jid_allreduce_lock.emplace(job->id, std::make_shared<resource<SIM_UNIT>>(sim, 1));

    for (unsigned iter = 0; iter < job->n_iter; ++iter) {
//        if (job->id == JID) printf("[1,%d]<stdout>:SYNTHETIC ITERATION START PROFILE %llu\n", id, sim.now());

        for (auto &tensor: tensors) {
            co_await tensor->lock.request(); // block if last step's allreduce is not completed yet
            tensor->allreduced_size = 0;
            auto fptime = forward_pass_time(tensor->size);
            auto forward_begin = sim.now();
            co_await sim.timeout(fptime);
//            if (PRINT) {
            printf("[forward] iter %d jid %d mid %d tid %d size %llu start %llu duration %llu end %llu\n", iter,
                   job->id, id, tensor->tensor_id, tensor->size, forward_begin, fptime, sim.now());
//            }
            tensor->lock.release();
        }
        //    printf("[%llu]\tWorker %d job %d forward pass all done\n", sim.now(), id, job->id);

        std::ranges::reverse_view reversed{tensors};
        unsigned tensor_id = 0;
        for (auto &tensor: reversed) {
            auto bptime = backward_pass_time(tensor->size);
            auto backward_begin = sim.now();
            co_await sim.timeout(bptime);
//            if (PRINT) {
            printf("[backward] iter %d jid %d mid %d tid %d size %d start %llu duration %llu end %llu\n", iter,
                   job->id, id, tensor->tensor_id, tensor->size, backward_begin, bptime, sim.now());
//            }
            co_await tensor->lock.request(); // release after allreduce is done so next fp can proceed
            printf("got lock for iter %d jid %d mid %d tid %d size %d\n", tensor->iter,
                   job->id, id, tensor->tensor_id, tensor->size);
            cs->enqueue(tensor);
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
        printf("waiting start for iter %d jid %d mid %d tid %d size %lu\n", tensor->iter,
               job->id, id, tensor->tensor_id, tensor->size);
        co_await tensor->lock.request(); // wait until final allreduces are done
        printf("waiting done for iter %d jid %d mid %d tid %d size %lu\n", tensor->iter,
               job->id, id, tensor->tensor_id, tensor->size);
    }
    job->finish_time = sim.now();
    gpu += gpus_required;
    printf("[%llu]\tmachine %d finishes job %d and has %d gpu now\n", sim.now(), id, job->id, gpu);
    assert(gpu <= gpu_capacity);
    cluster->check_if_all_jobs_finished();
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
//void Worker::allreduce(uint64_t) {
//    printf("[%llu] worker start allreduce\n", eventlist().now());
//    const Route *route = ((HierarchicalTopology *) cluster->topology())->get_worker_to_tor_path(_id, 0);
//    print_route(*route);
//    SwitchMLPacket *p = SwitchMLPacket::newpkt(*route);
//    p->job_id = 1;
//    p->wid = _id;
//    p->set_ts(eventlist().now());
//    printf("[%llu] worker sent packet\n", p->ts());
//    p->sendOn();
//}
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

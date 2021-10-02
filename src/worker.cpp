#include "worker.h"
#include "cluster.h"
#include "topology/hierarchical_topology.h"
#include "fschuetz04/simcpp20.hpp"

void Worker::doNextEvent() {
    // schedule start of allreduce
    allreduce(0);
}

void Worker::receivePacket(Packet &packet) {
    printf("[%llu] worker got packet\n", eventlist().now());
}

void Worker::allreduce(uint64_t) {
    printf("[%llu] worker start allreduce\n", eventlist().now());
    const Route *route = ((HierarchicalTopology *) cluster->topology())->get_worker_to_tor_path(_id, 0);
    print_route(*route);
    SwitchMLPacket *p = SwitchMLPacket::newpkt(*route);
    p->job_id = 1;
    p->wid = _id;
    p->set_ts(eventlist().now());
    printf("[%llu] worker sent packet\n", p->ts());
    p->sendOn();
}

void Worker::execute_job(const std::shared_ptr<Job> &job, const unsigned int gpus_required) {
    jobs.push_back(job);
    printf("[%llu]\tworker %d executing job %d using %d gpus\n", eventlist().now(), _id, job->id(), gpus_required);
    _gpu -= gpus_required;
    assert(_gpu >= 0);

//    std::vector<std::shared_ptr<Tensor>> tensors;
//    unsigned tensor_id = 0;
//    for (auto grad_size: job->model) {
//        tensors.push_back(
//                std::make_shared<Tensor>(Tensor{grad_size, 0, this, job, tensor_id++, 0, 0, resource(sim, 1)}));
//    }
//
//    auto &jid_allreduce_lock = allreduce_lock[id];
//    jid_allreduce_lock.erase(job->id);
//    jid_allreduce_lock.emplace(job->id, std::make_shared<resource<SIM_UNIT>>(sim, 1));
//
    for (unsigned iter = 0; iter < job->n_iter; ++iter) {
////        if (job->id == JID) printf("[1,%d]<stdout>:SYNTHETIC ITERATION START PROFILE %llu\n", id, sim.now());
//
//        for (auto &tensor: tensors) {
//            co_await tensor->lock.request(); // block if last step's allreduce is not completed yet
//            tensor->allreduced_size = 0;
//            auto fptime = forward_pass_time(tensor->size);
//            auto forward_begin = sim.now();
//            co_await sim.timeout(fptime);
//            if (PRINT) {
//                printf("[forward] iter %d jid %d mid %d tid %d size %llu start %llu duration %llu end %llu\n", iter,
//                       job->id, id, tensor->tensor_id, tensor->size, forward_begin, fptime, sim.now());
//            }
//            tensor->lock.release();
//        }
//        //    printf("[%llu]\tMachine %d job %d forward pass all done\n", sim.now(), id, job->id);
//
//        std::ranges::reverse_view reversed{tensors};
//        unsigned tensor_id = 0;
//        for (auto &tensor: reversed) {
//            auto bptime = backward_pass_time(tensor->size);
//            auto backward_begin = sim.now();
//            co_await sim.timeout(bptime);
//            if (PRINT) {
//                printf("[backward] iter %d jid %d mid %d tid %d size %d start %llu duration %llu end %llu\n", iter,
//                       job->id, id, tensor->tensor_id, tensor->size, backward_begin, bptime, sim.now());
//            }
//            co_await tensor->lock.request(); // release after allreduce is done so next fp can proceed
//            printf("got lock for iter %d jid %d mid %d tid %d size %d\n", tensor->iter,
//                   job->id, id, tensor->tensor_id, tensor->size);
//            cs->enqueue(tensor);
////            auto chunk_size = CHUNK_SIZE;
////            if (chunk_size) {
////                unsigned chunk_id = 0;
////                for (unsigned i = 0; i < tensor.size / chunk_size; ++i) {
////                    cs->enqueue(chunk_size, this, tensor_id, chunk_id++, job);
////                }
////                if (tensor.size % chunk_size) {
////                    cs->enqueue(tensor.size % chunk_size, this, tensor_id, chunk_id++, job);
////                }
////                tensor_id++;
////            } else {
////                cs->enqueue(tensor.size, this, tensor_id++, 0, job);
////            }
////            co_await sim.timeout(0);
////            allreduce_events.push_back(allreduce(sim, grad_size, tensor_id++, job)); // allreduce doesn't block
//        }
////        co_await sim.all_of(allreduce_events);
////        printf("[%llu]\tmachine %d backward loop over\n", sim.now(), id);
////        if(job->id==JID) printf("[1,%d]<stdout>:SYNTHETIC ITERATION END PROFILE %llu\n", id, sim.now());
    }
//    for (auto &tensor: tensors) {
//        printf("waiting start for iter %d jid %d mid %d tid %d size %lu\n", tensor->iter,
//               job->id, id, tensor->tensor_id, tensor->size);
//        co_await tensor->lock.request(); // wait until final allreduces are done
//        printf("waiting done for iter %d jid %d mid %d tid %d size %lu\n", tensor->iter,
//               job->id, id, tensor->tensor_id, tensor->size);
//    }
//    job->finish_time = sim.now();
//    gpu += gpus_required;
//    printf("[%llu]\tmachine %d finishes job %d and has %d gpu now\n", sim.now(), id, job->id, gpu);
//    assert(gpu <= gpu_capacity);
//    cluster->check_if_all_jobs_finished();

}

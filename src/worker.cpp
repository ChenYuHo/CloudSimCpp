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


simcpp20::event<SIM_UNIT>
Worker::execute_job(simcpp20::simulation<SIM_UNIT> &sim, std::shared_ptr<Job> job, const unsigned gpus_required,
                    CollectiveScheduler *cs) {
    jobs.push_back(job);
    myprintf("[%llu]\tmachine %d uses %d of its %d free GPUs for job %d \n", sim.now(), id, gpus_required, gpu,
             job->id);
    gpu -= gpus_required;
    assert(gpu >= 0);  // something wrong with the placement!
    job->start_time = sim.now();
    cluster->check_if_all_jobs_started();

    std::vector<std::shared_ptr<Tensor>> tensors;
    unsigned tensor_id = 0;
    if (job->model.size() == job->forward_pass_time.size()) {
        for (uint64_t i = 0; i < job->model.size(); ++i) {
            tensors.push_back(
                    std::make_shared<Tensor>(i, job->forward_pass_time[i], job->backward_pass_time[i], job->model[i],
                                             this, job, sim));
            locks_fp.emplace(job->id * 10000 + i,
                             make_shared<resource<SIM_UNIT>>(sim, 1));
            locks_allreduce.emplace(job->id * 10000 + i,
                                    make_shared<resource<SIM_UNIT>>(sim, 1));
        }
    } else {
        for (uint64_t i = 0; i < job->model.size(); ++i) {
            tensors.push_back(
                    std::make_shared<Tensor>(i, forward_pass_time(job->model[i]), backward_pass_time(job->model[i]),
                                             job->model[i], this, job, sim));
            locks_fp.emplace(job->id * 10000 + i,
                             make_shared<resource<SIM_UNIT>>(sim, 1));
            locks_allreduce.emplace(job->id * 10000 + i,
                                    make_shared<resource<SIM_UNIT>>(sim, 1));
        }
    }
//    auto tensor_counter = counter(sim, tensors.size(), tensors.size());
    for (unsigned iter = 0; iter < job->n_iter; ++iter) {
//        co_await tensor_counter.done();
        for (auto &tensor: tensors) {

//            myprintf("%d waiting LOCKF %p %llu %d\n", id, tensor.get(), tensor->tensor_id, tensor->iter);
            auto &lock = locks_fp[job->id * 10000 + tensor->tensor_id];

            myprintf(2, "mid %u tid %u jid %u forward request lock\n", id, tensor->tensor_id, job->id);
            co_await lock->request(); // will block if last step's allreduce is not completed yet
            myprintf(2, "mid %u tid %u jid %u forward got lock\n", id, tensor->tensor_id, job->id);
            if (tensor->tensor_id == 0) {
                myprintf("[1,%d]<stdout>:SYNTHETIC ITERATION START PROFILE %llu JOB %u\n", id, sim.now(), job->id);
            }
//            myprintf("%d LOCKF %llu %d\n", id, tensor->tensor_id, tensor->iter);
            tensor->allreduced_size = 0;
            auto fptime = tensor->forward_pass_time;
            auto forward_begin = sim.now();
            co_await sim.timeout(fptime);
//            if (PRINT) {
            myprintf("[forward] iter %d jid %d mid %d tid %llu size %llu start %llu duration %llu end %llu\n", iter,
                     job->id, id, tensor->tensor_id, tensor->size, forward_begin, fptime, sim.now());
//            }
            lock->release();
//            myprintf("%d ULOCKF %llu %d\n", id, tensor->tensor_id, tensor->iter);
        }
        //    myprintf("[%llu]\tWorker %d job %d forward pass all done\n", sim.now(), id, job->id);

        std::ranges::reverse_view reversed{tensors};
//        unsigned tensor_id = 0;
        for (auto &tensor: reversed) {
//            auto bptime = backward_pass_time(tensor->size);
            auto bptime = tensor->backward_pass_time;
            auto backward_begin = sim.now();
            co_await sim.timeout(bptime);
            myprintf("[backward] iter %d jid %d mid %d tid %d size %d start %llu duration %llu end %llu\n", iter,
                     job->id, id, tensor->tensor_id, tensor->size, backward_begin, bptime, sim.now());
            auto key = job->id * 10000 + tensor->tensor_id;
            co_await locks_fp[key]->request(); // release after allreduce is done so next fp can proceed
//            myprintf("%d LOCKB %llu %d\n", id, tensor->tensor_id, tensor->iter);
            //            myprintf("got lock for iter %d jid %d mid %d tid %d size %d\n", tensor->iter,
//                   job->id, id, tensor->tensor_id, tensor->size);

            tensor->iter++;
            if (job->num_workers_allocated > 1) {
                // original
                if (cs != nullptr) {
                    cs->enqueue(sim, tensor);
                } else {
                    myprintf("mid %u invoking nonblocking allreduce\n", id);
                    allreduce(sim, tensor); // nonblocking
                }
            } else {
//                myprintf("job %u single machine, no allreduce\n", job->id);
                locks_fp[key]->release();
                if (id == tensor->job->master_mid) {
//                    myprintf("incrementing cpb from %d\n", id);
                    cpb.cntIncrement();
                }
            }





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
//        myprintf("[%llu]\tmachine %d backward loop over\n", sim.now(), id);
//        if(job->id==JID) myprintf("[1,%d]<stdout>:SYNTHETIC ITERATION END PROFILE %llu\n", id, sim.now());
    }
    for (auto &tensor: tensors) {
//        myprintf("waiting start for iter %d jid %d mid %d tid %llu size %llu", tensor->iter,
//               job->id, id, tensor->tensor_id, tensor->size);
        auto key = job->id * 10000 + tensor->tensor_id;
        auto &lock = locks_fp[key];
        co_await lock->request(); // wait until final allreduces are done
        lock->release();
        locks_fp.erase(key);
        locks_allreduce.erase(key);
//        locks.erase(job->id * 10000 + tensor->tensor_id);
//        myprintf("%d LOCKFINAL %llu %d\n", id, tensor->tensor_id, tensor->iter);
//        myprintf("waiting done for iter %d jid %d mid %d tid %llu size %llu\n", tensor->iter,
//               job->id, id, tensor->tensor_id, tensor->size);
    }
    tensors.clear();
    job->finish_time = sim.now();
    gpu += gpus_required;
    myprintf("[%lu]\tmachine %d finishes job %d and has %d gpu now\n", sim.now(), id, job->id, gpu);
    assert(gpu <= gpu_capacity);
    cluster->check_if_all_jobs_finished();
}

void Worker::doNextEvent() {

}

void Worker::receivePacket(Packet &pkt) {
    auto p = (SwitchMLPacket *) &pkt;
//    myprintf("[%lu] worker %d got aggregated pkt JID %d slot %d offset %d\n",
//           eventlist().now(), id, p->job_id, p->slot, p->offset);
    unsigned pkt_idx = p->offset / NUM_UPDATES;
    auto &set = received_pkts[p->tensor->tensor_id];
    if (set.contains(pkt_idx)) {
        p->free();
        // duplicate
//        myprintf("already received %d/%d pkt, discarding\n", pkt_idx, p->tensor->num_pkts_expected);
        return;
    }
    set.insert(pkt_idx);
    // cancel timer
    if (set.size() == p->tensor->num_pkts_expected) {
        auto key = p->job_id * 10000 + p->tensor->tensor_id;
        // can't erase if loss recovery is enabled
        received_pkts.erase(p->tensor->tensor_id); // counter for worker
        locks_allreduce[key]->release();
        p->free();
        return;
    }
    auto next_start = p->offset + NUM_UPDATES * NUM_SLOTS;
    if (next_start >= p->grad_size) {
        p->free();
        return;
    }
    sendPacket(next_start, 1 - p->ver, p->slot, p->grad_size, p->tensor);
//    myprintf("[%llu] worker %d got aggregated pkt JID %d slot %d offset %d\n",
//           eventlist().now(), id, p->job_id, p->slot, p->offset);
    p->free();
}


void Worker::sendPacket(unsigned start, unsigned ver,
                        unsigned slot, unsigned grad_size,
                        shared_ptr<Tensor> tensor) {
    auto topo = cluster->_topo;
    const Route *route = topo->get_worker_to_tor_path(id);
    auto p = SwitchMLPacket::newpkt(*route);
    p->set_packet_size(SWITCHML_PKT_SIZE);
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
//    p->print_info(0, eventlist().now(), 0);
//    p->cnt+=1;
//    print_route(*route);
//    myprintf("worker sent pkt\n");
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
                                            shared_ptr<Tensor> tensor,
                                            unsigned chunk_size) {
    // assuming a chunked tensor does not concurrently invoke allreduce
    // i.e., at most one allreduce going on for a tensor

    auto key = tensor->job->id * 10000 + tensor->tensor_id;
    co_await locks_allreduce[key]->request();
    allreduce_start[key] = sim.now();
    auto grad_size = (chunk_size == 0)
                     ? tensor->size
                     : (tensor->size - tensor->allreduced_size > chunk_size)
                       ? chunk_size
                       : tensor->size - tensor->allreduced_size;
    if (SIMULATE_ALLREDUCE) {
//    myprintf("[%llu] worker %d start allreduce for tensor size %llu iter %d\n", eventlist().now(), id, grad_size,
//           tensor->iter);
        const static unsigned num_updates = (SWITCHML_PKT_SIZE - (8 + 14 + 20 + 8 + 6 + 4 + 12)) / 4;
        tensor->num_pkts_expected = grad_size / num_updates;
        if (grad_size % num_updates) tensor->num_pkts_expected += 1;

        // assuming switches have infinite slots
        for (unsigned slot = 0; slot < NUM_SLOTS; ++slot) {
            auto start = slot * num_updates;
            if (start >= grad_size) break;
            sendPacket(start, 0, slot, grad_size, tensor);
//            myprintf("[%llu] worker %d send packet %d/%d for tensor size %llu iter %d to slot %d\n", eventlist().now(), id, slot, tensor->num_pkts_expected, grad_size,
//                     tensor->iter, slot);
        }
    } else {
        SIM_UNIT sleep_time = 0;
        switch (grad_size) {
            case 1000:
            case 4096:
            case 256:
            case 384:
            case 192:
            case 64:
            case 23232:
            case 25393:
                sleep_time = 4000000;
                break;
            case 4096000:
                sleep_time = 128000000;
                break;
            case 16777216:
                sleep_time = 512000000;
                break;
            case 589824:
                sleep_time = 20000000;
                break;
            case 884736:
                sleep_time = 28000000;
                break;
            case 663552:
                sleep_time = 24000000;
                break;
            case 307200:
                sleep_time = 12000000;
                break;
            case 37748736:
                sleep_time = 1152000000;
                break;
            default:;
        }
        co_await sim.timeout(sleep_time);
        locks_allreduce[key]->release();
    }
    co_await locks_allreduce[key]->request();
    tensor->allreduced_size += grad_size;
    auto end = sim.now();
    myprintf("[allreduce] iter %d jid %d mid %d tid %d size %d start %llu duration %llu end %llu\n",
             tensor->iter - 1, tensor->job->id, id, tensor->tensor_id, grad_size, allreduce_start[key],
             end - allreduce_start[key], end);
    locks_allreduce[key]->release();
    if (tensor->allreduced_size == tensor->size) {
        locks_fp[key]->release();
        if (id == tensor->job->master_mid) {
//        myprintf("incrementing cpb from %d\n", id);
            cpb.cntIncrement();
        }
    }

}

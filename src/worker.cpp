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


inline uint64_t get_key(uint64_t job_id, uint64_t tensor_id) {
    return job_id * 1000000 + tensor_id;
}

simcpp20::event<SIM_UNIT>
Worker::execute_job(simcpp20::simulation<SIM_UNIT> &sim, std::shared_ptr<Job> job, const unsigned gpus_required,
                    CollectiveScheduler *cs) {
    auto rank = rank_for_job[job->id];
    jobs.push_back(job);
    myprintf("[%llu]\tmid %u rank %u uses %d out of %d free GPUs for job %d \n", sim.now(), id, rank, gpus_required,
             gpu, job->id);
    gpu -= gpus_required;
    assert(gpu >= 0);  // something wrong with the placement!

    std::vector<std::shared_ptr<Tensor>> tensors;
    auto has_timing = job->model.size() == job->forward_pass_time.size();
    for (uint64_t i = 0; i < job->model.size(); ++i) {
        auto tensor = make_shared<Tensor>(i,
                                          has_timing ? job->forward_pass_time[i] : forward_pass_time(job->model[i]),
                                          has_timing ? job->backward_pass_time[i] : backward_pass_time(job->model[i]),
                                          job->model[i], this, job, sim);
        tensors.push_back(tensor);
        fp_locks.emplace(get_key(job->id, i), make_shared<resource<SIM_UNIT>>(sim, 1));
        allreduce_locks.emplace(get_key(job->id, i), make_shared<resource<SIM_UNIT>>(sim, 1));
    }
    for (unsigned iter = 0; iter < job->n_iter; ++iter) {
        for (auto &tensor: tensors) {
            auto key = get_key(job->id, tensor->tensor_id);
            myprintf(2, "L38 mid %u rank %u tid %u jid %u forward request lock\n", id, rank, tensor->tensor_id,
                     job->id);
            co_await fp_locks[key]->request(); // will block if last step's allreduce is not completed yet
            myprintf(2, "L40 mid %u rank %u tid %u jid %u forward got lock\n", id, rank, tensor->tensor_id, job->id);
            if (tensor->tensor_id == 0) { // mark iteration start
                myprintf(4, "[%u,%u]<stdout>:SYNTHETIC ITERATION START PROFILE %llu\n", job->id, rank, sim.now());
            }
            auto fptime = tensor->forward_pass_time;
            auto forward_begin = sim.now();
            co_await sim.timeout(fptime);
            myprintf(4, "[forward] iter %d jid %d mid %d tid %llu size %llu start %llu duration %llu end %llu\n", iter,
                     job->id, id, tensor->tensor_id, tensor->size, forward_begin, fptime, sim.now());
            fp_locks[key]->release();
            myprintf(2, "L50 mid %u rank %u tid %u jid %u release lock\n", id, rank, tensor->tensor_id, job->id);
        }
        std::ranges::reverse_view reversed{tensors};
        for (auto &tensor: reversed) {
            auto key = get_key(job->id, tensor->tensor_id);
            auto bptime = tensor->backward_pass_time;
            auto backward_begin = sim.now();
            co_await sim.timeout(bptime);
            myprintf(4, "[backward] iter %d jid %d mid %d tid %d size %d start %llu duration %llu end %llu\n", iter,
                     job->id, id, tensor->tensor_id, tensor->size, backward_begin, bptime, sim.now());

            myprintf(2, "L60 mid %u rank %u tid %u jid %u backward request lock\n", id, rank, tensor->tensor_id,
                     job->id);
            co_await fp_locks[key]->request(); // release after allreduce is done so next fp can proceed
            myprintf(2, "L62 mid %u rank %u tid %u jid %u backward got lock\n", id, rank, tensor->tensor_id, job->id);
            if (job->num_workers_allocated > 1) {

                if (cs != nullptr) {
                    cs->enqueue(sim, tensor);
                } else {
                    myprintf(2, "L67 mid %u rank %u tid %u jid %u calling allreduce\n", id, rank, tensor->tensor_id,
                             job->id);
                    allreduce(sim, tensor); // nonblocking
                }
            } else {
                fp_locks[key]->release();
                myprintf(2, "L72 mid %u rank %u tid %u jid %u released lock\n", id, rank, tensor->tensor_id, job->id);
                if (rank_for_job[tensor->job->id] == 0) {
//                    myprintf("incrementing cpb from %d\n", id);
                    myprintf(2, "L75 mid %u rank %u tid %u jid %u incrementing progress\n", id, rank, tensor->tensor_id,
                             job->id);
                    cpb.cntIncrement();
                }
            }
        }
    }
    for (auto &tensor: tensors) {
        auto key = get_key(job->id, tensor->tensor_id);
        co_await fp_locks[key]->request(); // wait until final allreduces are done
        fp_locks[key]->release();
    }
    tensors.clear();

    gpu += gpus_required;
    myprintf(4, "[%lu]\tmachine %d finishes job %d and has %d gpu now\n", sim.now(), id, job->id, gpu);
    if (rank_for_job[job->id] == 0) {
        job->finish_time = sim.now();
        cluster->check_if_all_jobs_finished();
    }
}

void Worker::doNextEvent() {

}

void Worker::receivePacket(Packet &pkt) {
    auto p = (SwitchMLPacket *) &pkt;
    auto key = get_key(p->job_id, p->tensor->tensor_id);
    myprintf(8, "[%lu] worker %d got aggregated pkt iter %u JID %d tid %u slot %d offset %d\n",
             eventlist().now(), id, p->tensor->iter, p->job_id, p->tensor->tensor_id, p->slot, p->offset);

    ///////// tensor is the same !!!!!!!!!

    auto &set = received_pkts[key];
    if (set.contains(p->offset)) {
        p->free();
        // duplicate
        myprintf(8, "already received %d/%d pkt, discarding\n", p->offset / NUM_UPDATES, p->tensor->num_pkts_expected);
        return;
    }
    set.insert(p->offset);
    // cancel timer
    myprintf(8, "%u, %u, %u, %u\n", id, p->offset, set.size(), p->tensor->num_pkts_expected);
    if (set.size() == p->tensor->num_pkts_expected) {
        // can't erase if loss recovery is enabled
        set.clear();
        allreduce_locks[key]->release();
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
//    p->set_packet_size(SWITCHML_PKT_SIZE);
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
    auto key = get_key(tensor->job->id, tensor->tensor_id);
    // assuming a chunked tensor does not concurrently invoke allreduce
    // i.e., at most one allreduce going on for a tensor
    myprintf(2, "L166 mid %u tid %u jid %u allreduce request lock\n", id, tensor->tensor_id, tensor->job->id);
    co_await allreduce_locks[key]->request();
    myprintf(2, "L168 mid %u tid %u jid %u allreduce got lock\n", id, tensor->tensor_id, tensor->job->id);

    tensor->allreduce_start = sim.now();
    auto grad_size = (chunk_size == 0)
                     ? tensor->size
                     : (tensor->size - tensor->allreduced_size > chunk_size)
                       ? chunk_size
                       : tensor->size - tensor->allreduced_size;
    if (SIMULATE_ALLREDUCE) {
    myprintf(8, "[%llu] worker %d start allreduce for tensor size %llu iter %d\n", eventlist().now(), id, grad_size,
           tensor->iter);
        tensor->num_pkts_expected = grad_size / NUM_UPDATES;
        if (grad_size % NUM_UPDATES) tensor->num_pkts_expected += 1;

        // assuming switches have infinite sets of slots
        for (unsigned slot = 0; slot < NUM_SLOTS; ++slot) {
            auto start = slot * NUM_UPDATES;
            if (start >= grad_size) break;
            sendPacket(start, 0, slot, grad_size, tensor);
            myprintf(8, "[%llu] worker %d send packet %d/%d for tensor size %llu iter %d to slot %d\n", eventlist().now(), id, slot, tensor->num_pkts_expected, grad_size,
                     tensor->iter, slot);
        }
//    } else {
//        SIM_UNIT sleep_time = 0;
//        switch (grad_size) {
//            case 1000:
//            case 4096:
//            case 256:
//            case 384:
//            case 192:
//            case 64:
//            case 23232:
//            case 25393:
//                sleep_time = 4000000;
//                break;
//            case 4096000:
//                sleep_time = 128000000;
//                break;
//            case 16777216:
//                sleep_time = 512000000;
//                break;
//            case 589824:
//                sleep_time = 20000000;
//                break;
//            case 884736:
//                sleep_time = 28000000;
//                break;
//            case 663552:
//                sleep_time = 24000000;
//                break;
//            case 307200:
//                sleep_time = 12000000;
//                break;
//            case 37748736:
//                sleep_time = 1152000000;
//                break;
//            default:;
//        }
//        co_await sim.timeout(sleep_time);
//        tensor->allreduce_lock.release();
    }
    myprintf(2, "L227 mid %u tid %u jid %u allreduce request lock\n", id, tensor->tensor_id, tensor->job->id);
    co_await allreduce_locks[key]->request();
    myprintf(2, "L231 mid %u tid %u jid %u allreduce got lock\n", id, tensor->tensor_id, tensor->job->id);
    tensor->allreduced_size += grad_size;
    auto end = sim.now();
    myprintf(8, "[allreduce] iter %d jid %d mid %d tid %d size %d start %llu duration %llu end %llu\n",
             tensor->iter, tensor->job->id, id, tensor->tensor_id, grad_size, tensor->allreduce_start,
             end - tensor->allreduce_start, end);
    allreduce_locks[key]->release();
    if (tensor->allreduced_size >= tensor->size) {
        myprintf(8, "RANK %u mid %u jid %u\n", rank_for_job[tensor->job->id], id, tensor->job->id);
        tensor->iter++;
        tensor->allreduced_size = 0;
        fp_locks[key]->release();
        myprintf(2, "L316 mid %u jid %u tid %u release lock locks_fp %d\n", id, tensor->job->id, tensor->tensor_id);
        if (rank_for_job[tensor->job->id] == 0) {
            myprintf(8, "incrementing cpb from %d\n", id);
            cpb.cntIncrement();
        }
    }
}

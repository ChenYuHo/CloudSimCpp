#include <vector>
#include <memory>
#include <sstream>
#include <ranges>
#include "common.h"
#include "worker.h"
#include "job.h"
#include "switch.h"
#include "cluster.h"
#include "packet.h"
#include <glog/logging.h>
#include <algorithm>

// verbose loggings:
// 2: iterations
// 3: (DEBUG) forward backward allreduce times

std::string to_string(const std::vector<SIM_UNIT> &collective_timings) {
    if (collective_timings.empty()) return "";
    std::ostringstream out;
    std::copy(collective_timings.begin(),
              collective_timings.end() - 1,
              std::ostream_iterator<SIM_UNIT>(out, ","));
    out << collective_timings.back();
    return out.str();
}

simcpp20::event<SIM_UNIT>
Worker::execute_job(simcpp20::simulation<SIM_UNIT> &sim, Job *job, unsigned gpus_required, CollectiveScheduler *cs) {
    auto rank = rank_for_job[job->id];
    jobs.push_back(job);
    myprintf(3, "[%llu]\tmid %u rank %u uses %d out of %d free GPUs for job %d \n", sim.now(), id, rank, gpus_required,
             gpu, job->id);
    gpu -= gpus_required;
    CHECK_GE(gpu, 0) << ": something wrong with the placemnet";

    std::vector<Tensor *> tensors;
    auto has_timing = job->model.size() == job->forward_pass_time.size();
    for (uint64_t i = 0; i < job->model.size(); ++i) {
        auto tensor = new Tensor(i,
                                 std::stoul(getenv("NO_COMPUTATION_TIME", "0"))
                                 ? 0
                                 : (has_timing
                                    ? job->forward_pass_time[i]
                                    : forward_pass_time(job->model[i])),
                                 std::stoul(getenv("NO_COMPUTATION_TIME", "0"))
                                 ? 0
                                 : (has_timing
                                    ? job->backward_pass_time[i]
                                    : backward_pass_time(job->model[i])),
                                 job->model[i], this, job, sim);
        tensors.push_back(tensor);
        fp_locks.emplace(tensor->key, new resource<SIM_UNIT>(sim, 1));
        allreduce_locks.emplace(tensor->key, new resource<SIM_UNIT>(sim, 1));
    }
    for (int iter = 0; iter < job->n_iter; ++iter) {
        job->in_iter = iter;
        for (auto &tensor: tensors) {
            myprintf(2, "L38 mid %u rank %u tid %u jid %u forward request lock\n",
                     id, rank, tensor->tensor_id, job->id);
            co_await fp_locks[tensor->key]->request(); // will block if last step's allreduce is not completed yet
            myprintf(2, "L40 mid %u rank %u tid %u jid %u forward got lock\n",
                     id, rank, tensor->tensor_id, job->id);
            if (tensor->tensor_id == 0 && rank_for_job[tensor->job->id] == 0) { // mark iteration start
                myprintf(4, "[%u,%u]<stdout>:SYNTHETIC ITERATION START PROFILE %llu\n",
                         job->id, rank, sim.now());
            }
            auto fptime = tensor->forward_pass_time;
            auto forward_begin = sim.now();
            job->fp_layer = tensor->tensor_id;
            co_await sim.timeout(fptime);
            if (rank_for_job[tensor->job->id] == 0)
                myprintf(4,
                         "[forward] iter %d jid %d mid %d rank %u tid %u size %llu start %llu duration %llu end %llu\n",
                         iter, job->id, id, rank, tensor->tensor_id, tensor->size, forward_begin, fptime, sim.now());
        }
        for (auto &tensor: tensors | std::ranges::views::reverse) {
            auto bptime = tensor->backward_pass_time;
            auto backward_begin = sim.now();
            co_await sim.timeout(bptime);
            if (rank_for_job[tensor->job->id] == 0)
                myprintf(4,
                         "[backward] iter %d jid %d mid %d rank %u tid %d size %d start %llu duration %llu end %llu\n",
                         iter, job->id, id, rank, tensor->tensor_id, tensor->size, backward_begin, bptime, sim.now());
            if (job->num_workers_allocated > 1) {
                if (cs) {
                    if (rank_for_job[job->id] == 0) {
//                        myprintf(5, "&\t%lu\n", sim.now());
                        if (COLLECTIVE_STATISTICS) {
                            if (!tensor->collective_timings.empty()) {
                                myprintf("#CT j %u t %u i %u %s\n", job->id, tensor->tensor_id, iter - 1,
                                         to_string(tensor->collective_timings).c_str());
                                tensor->collective_timings.clear();
                            }
                            tensor->collective_timings.push_back(sim.now());
                        }
                    }
                    cs->enqueue(sim, tensor);
                } else {
//                    if (rank_for_job[job->id] == 0) {
//                        myprintf(5, "&\t%lu\n", sim.now());
//                    }
                    myprintf(2, "L67 mid %u rank %u tid %u jid %u calling allreduce\n", id, rank, tensor->tensor_id,
                             job->id);
                    allreduce(sim, tensor); // nonblocking
                }
            } else {
                fp_locks[tensor->key]->release();
                if (tensor->tensor_id == 0 && rank_for_job[tensor->job->id] == 0) { // mark iteration end
                    myprintf(4, "[%u,%u]<stdout>:SYNTHETIC ITERATION END PROFILE %llu\n", job->id, rank, sim.now());
                }
                myprintf(2, "L72 mid %u rank %u tid %u jid %u released lock\n", id, rank, tensor->tensor_id, job->id);
                if (rank_for_job[tensor->job->id] == 0) {
                    myprintf(2, "L75 mid %u rank %u tid %u jid %u incrementing progress\n",
                             id, rank, tensor->tensor_id, job->id);
                    cpb.cntIncrement();
                }
            }
        }
    }
    for (auto &tensor: tensors) {
        co_await fp_locks[tensor->key]->request(); // wait until final allreduces are done
        co_await allreduce_locks[tensor->key]->request();
        if (job->num_workers_allocated > 1 && COLLECTIVE_STATISTICS && rank_for_job[job->id] == 0) {
            myprintf("#CT j %u t %u i %u %s\n", job->id, tensor->tensor_id, job->n_iter,
                     to_string(tensor->collective_timings).c_str());
        }
        fp_locks[tensor->key]->release();
        allreduce_locks[tensor->key]->release();
    }

    co_await sim.timeout(0);
    // job is done! clean a bit...
    // clean collective scheduler
    if (cs) cs->cleanup_for_job(job->id);

    gpu += gpus_required;
    myprintf(3, "[%lu]\tmachine %d finishes job %d and has %d gpu now\n", sim.now(), id, job->id, gpu);
    if (rank_for_job[job->id] == 0) {
        job->finish_time = sim.now();
        if (cluster) cluster->check_if_all_jobs_finished();
    }

    for (auto &tensor: tensors) {
        delete fp_locks[tensor->key];
        delete allreduce_locks[tensor->key];
        delete tensor;
    }
    tensors.clear();
}

void Worker::doNextEvent() {}

void Worker::receivePacket(Packet &pkt) {
    auto p = (SwitchMLPacket *) &pkt;
    myprintf(8, "[%lu] worker %d got aggregated pkt iter %u JID %d tid %u slot %d offset %d\n",
             eventlist().now(), id, p->tensor->iter, p->job_id, p->tensor->tensor_id, p->slot, p->offset);

    auto &set = received_pkts[p->tensor->key];
    if (set.contains(p->offset)) {
        // duplicate
        myprintf(8, "already received %d/%d pkt, discarding\n", p->offset / NUM_UPDATES, p->tensor->num_pkts_expected);
        p->free();
        return;
    }
    set.insert(p->offset);
    // cancel timer
    if (set.size() == p->tensor->num_pkts_expected) {
        // can't erase if loss recovery is enabled
        set.clear();
        allreduce_locks[p->tensor->key]->release();
        received_pkts.erase(p->tensor->key);
        p->free();
        return;
    }
    auto next_start = p->offset + NUM_UPDATES * NUM_SLOTS;
    if (next_start >= p->grad_size) {
        p->free();
        return;
    }
    sendPacket(next_start, 1 - p->ver, p->slot, p->grad_size, p->tensor);
    p->free();
}


void Worker::sendPacket(unsigned start, unsigned ver,
                        unsigned slot, unsigned grad_size,
                        Tensor *tensor) {
    auto topo = cluster->_topo;
//    auto route = topo->get_worker_to_tor_path(id);
    auto p = SwitchMLPacket::newpkt(*route_to_tor);
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
//    cout << fmt::format("Worker{} ", id);
//    print_route(*route);
//    myprintf("worker sent pkt\n");
    p->sendOnSimple();
}

simcpp20::event<SIM_UNIT> Worker::allreduce(simcpp20::simulation<SIM_UNIT> &sim,
                                            Tensor *tensor,
                                            uint64_t chunk_size) {
    if (rank_for_job[tensor->job->id] == 0) {
        // allreduce start
        myprintf(5, "{\t%lu\t%u\t%u\n", sim.now(), tensor->job->id, tensor->tensor_id);
        if (COLLECTIVE_STATISTICS) {
            tensor->collective_timings.push_back(sim.now());
        }
    }
    auto rank = rank_for_job[tensor->job->id];
    // assuming a chunked tensor does not concurrently invoke allreduce
    // i.e., at most one allreduce going on for a tensor
    myprintf(2, "L166 mid %u tid %u jid %u allreduce request lock\n", id, tensor->tensor_id, tensor->job->id);
    co_await allreduce_locks[tensor->key]->request();
    myprintf(2, "L168 mid %u tid %u jid %u allreduce got lock\n", id, tensor->tensor_id, tensor->job->id);

    tensor->allreduce_start = sim.now();
    auto grad_size = (chunk_size == 0)
                     ? tensor->size
                     : std::min(tensor->size - tensor->allreduced_size, chunk_size);
    myprintf(8, "[%llu] mid %u tid %u jid %u allreduce size %llu iter %d\n", eventlist().now(), id,
             tensor->tensor_id, tensor->job->id, grad_size, tensor->iter);
    tensor->num_pkts_expected = grad_size / NUM_UPDATES;
    if (grad_size % NUM_UPDATES) tensor->num_pkts_expected += 1;

#ifndef NOSIMPKT
    // assuming switches have infinite sets of slots
    for (unsigned slot = 0; slot < NUM_SLOTS; ++slot) {
        auto start = slot * NUM_UPDATES;
        if (start >= grad_size) break;
        sendPacket(start, 0, slot, grad_size, tensor);
        myprintf(8,
                 "[%llu] mid %u tid %u jid %u allreduce send packet %d/%d for tensor size %llu iter %d to slot %d offset %u\n",
                 eventlist().now(), id, tensor->tensor_id, tensor->job->id, slot, tensor->num_pkts_expected,
                 grad_size, tensor->iter, slot, slot * NUM_SLOTS);
    }
#else
    co_await sim.timeout(grad_size * (32 * 1000000 / HOST_NIC)); // grad_size*32bits*Mbps*1e6/1e12
    allreduce_locks[tensor->key]->release();
#endif
    myprintf(2, "L227 mid %u tid %u jid %u allreduce request lock\n", id, tensor->tensor_id, tensor->job->id);
    co_await allreduce_locks[tensor->key]->request();
    myprintf(2, "L231 mid %u tid %u jid %u allreduce got lock\n", id, tensor->tensor_id, tensor->job->id);
    tensor->allreduced_size += grad_size;
    auto end = sim.now();
    if (rank_for_job[tensor->job->id] == 0) {
        // allreduce end
        myprintf(5, "}\t%lu\t%u\t%u\n", sim.now(), tensor->job->id, tensor->tensor_id);
        myprintf(4, "[allreduce] iter %d jid %d mid %d rank %u tid %d size %d start %llu duration %llu end %llu\n",
                 tensor->iter, tensor->job->id, id, rank, tensor->tensor_id, grad_size, tensor->allreduce_start,
                 end - tensor->allreduce_start, end);
    }
    allreduce_counter[tensor->iter % 2]++;
    if (allreduce_counter[tensor->iter % 2] == tensor->job->model.size()) { // all allreduces done, mark iteration end
        allreduce_counter[1 - tensor->iter % 2] = 0;
//        co_await sim.timeout(0); // TODO: post allreduce processing time: weight update etc
        myprintf(4, "[%u,%u]<stdout>:SYNTHETIC ITERATION END PROFILE %llu\n",
                 tensor->job->id, rank, sim.now());
    }
    allreduce_locks[tensor->key]->release();
    if (tensor->allreduced_size >= tensor->size) {
        myprintf(8, "RANK %u mid %u tid %u jid %u\n", rank, id, tensor->tensor_id, tensor->job->id);
        tensor->iter++;
        tensor->allreduced_size = 0;
        tensor->chunk_id = 0;
        fp_locks[tensor->key]->release();
        myprintf(2, "L316 mid %u tid %u jid %u release lock locks_fp\n", id, tensor->tensor_id);
        if (rank == 0) {
            myprintf(8, "incrementing cpb from %d\n", id);
            cpb.cntIncrement();
        }
        // clean Switch status
        if (clean_ToR_for_job[tensor->job->id] && tor) {
            tor->count_for_tensor_key.erase(tensor->key);
            tor->seen_for_tensor_key.erase(tensor->key);
            if (tor->clean_upper_level_switch_for_job[tensor->job->id] && tor->upper_level_switch) {
                tor->upper_level_switch->count_for_tensor_key.erase(tensor->key);
                tor->upper_level_switch->seen_for_tensor_key.erase(tensor->key);
            }
        }
    } else {
        tensor->chunk_id++;
        myprintf(8, "RANK %u mid %u tid %u jid %u allreduced %u\n", rank, id, tensor->tensor_id, tensor->job->id,
                 tensor->allreduced_size);
    }
}

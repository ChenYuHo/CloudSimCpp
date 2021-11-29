#include "deficit_round_robin.h"
#include "job.h"
#include "cluster.h"
#include "worker.h"

simcpp20::event<SIM_UNIT>
DeficitRoundRobin::enqueue(simcpp20::simulation<SIM_UNIT> &sim, Tensor *tensor) {
    queue[tensor->key].push_back(tensor);
    if (queue[tensor->key].size() == tensor->job->num_workers_allocated) {
        myprintf(7, "putting jid %u tid %u size %u into ready queue\n", tensor->job->id, tensor->tensor_id,
                 tensor->size);
        ready_queue[tensor->job->id].push(std::move(queue[tensor->key]));
        queue.erase(tensor->key);
        if (!active_jobs.contains(tensor->job->id)) { //  && !done_jobs.contain(tensor->job->id)
            active_jobs.insert(tensor->job->id);
            kick_off(sim, tensor->job->id);
        }
    }
    co_await sim.timeout(0);
}

simcpp20::event<SIM_UNIT> DeficitRoundRobin::kick_off(simcpp20::simulation<SIM_UNIT> &sim, unsigned jid) {
    auto &pq = ready_queue[jid];
    while (active_jobs.contains(jid)) {
        if (pq.empty()) {
            co_await sim.timeout(timeFromUs(5u));
            continue;
        }
        auto tensors = pq.top();
        auto front = tensors.front();
        if (front->allreduced_size + CHUNK_SIZE >= front->size) {
            myprintf(7, "popping jid %u tid %u\n",
                     jid, front->tensor_id);
            pq.pop();
        }
        std::vector<simcpp20::event<SIM_UNIT>> allreduce_events{};
        myprintf(7, "invoking allreduce for jid %u tid %u iter %u cid %u/%u\n",
                 jid, front->tensor_id, front->iter, front->allreduced_size / CHUNK_SIZE,
                 front->size % CHUNK_SIZE ? front->size / CHUNK_SIZE +1 : front->size / CHUNK_SIZE);
        for (auto tensor:tensors) {
            allreduce_events.push_back(std::move(tensor->machine->allreduce(sim, tensor, CHUNK_SIZE)));
        }
        co_await sim.all_of(allreduce_events);
    }
}

simcpp20::event<SIM_UNIT> DeficitRoundRobin::collective_scheduler(
        simcpp20::simulation<SIM_UNIT> &sim,
        Cluster &cluster) {
    while (!cluster.all_jobs_finished) {
        //            printf("scanning through queue\n");
        for (auto &entry: ready_queue) {
            auto job_id = entry.first;
            auto &pqueue = entry.second; // priority queue
            auto drr = quantums[job_id];
            drr += quantum;
            while (!pqueue.empty()) {
                printf("job %d drr %f\n", job_id, drr);
                auto tensors = pqueue.top();
                auto q = compute_quantum(tensors.front());
                //                    double q = 1.;
                if (drr >= q) {
                    drr -= q;
                    std::vector<simcpp20::event<SIM_UNIT>> allreduce_events;
                    uint64_t allreduce_size;
                    for (auto &tensor: tensors) {
                        if (tensor->size - tensor->allreduced_size > chunk_size) {
                            allreduce_size = chunk_size;
                        } else {
                            allreduce_size = tensor->size % chunk_size;
                        }
                        allreduce_events.push_back(
                                tensor->machine->allreduce(sim, allreduce_size,
                                                           std::to_string(tensor->tensor_id) + "," +
                                                           std::to_string(tensor->chunk_id), tensor->job));
                        tensor->allreduced_size += allreduce_size;
                        tensor->chunk_id++;
                    }
                    auto begin = sim.now();
                    co_await sim.all_of(allreduce_events);
                    auto end = sim.now();
                    for (auto &tensor : tensors) {
                        if (PRINT) {
                            printf("[allreduce] iter %d jid %d mid %d tid %d size %lu start %llu duration %llu end %llu cid %d\n",
                                   tensor->iter, tensor->job->id, tensor->machine->id, tensor->tensor_id,
                                   allreduce_size, begin, end - begin, end, tensor->chunk_id);
                        }
                    }
                    printf("=== allreduced %lu %lu\n", tensors.front()->allreduced_size, tensors.front()->size);
                    if (tensors.front()->allreduced_size >= tensors.front()->size) {
                        for (auto &tensor : tensors) {
                            tensor->iter++;
                            tensor->lock.release();
                        }
                        tensors.clear();
                        pqueue.pop();
                    }
                    //                        unsigned count = 0;

                    //                        for (auto &p : pqueue) { // check all worker enqueued
                    //                            if (p.tid == pkt.tid && p.cid == pkt.cid) count++;
                    //                            if (count == pkt.job->num_workers_allocated) break;
                    //                        }
                    //                        if (count == pkt.job->num_workers_allocated) {
                    //                            for (auto it = pqueue.begin(); it != pqueue.end();) {
                    //                                auto &p = *it;
                    //                                if (p.tid == pkt.tid && p.cid == pkt.cid) {
                    //                                    printf("[%llu]\tinvoking allreduce within cs s%llu t%d c%d j%d m%d\n", sim.now(),
                    //                                           p.size, p.tid, p.cid, p.job->id, p.machine->id);
                    //                                    allreduce_events.push_back(p.machine->allreduce(sim, p.size, p.tid, p.cid, p.job));
                    //                                    it = pqueue.erase(it);
                    //                                } else {
                    //                                    ++it;
                    //                                }
                    //                            }
                    //                        }
                    //                        co_await sim.all_of(allreduce_events);
                    //                        pkt.machine->allreduce(sim, pkt.size, pkt.tid, pkt.job);
                    //                        pqueue.pop_front();
                } else break;
            }
        }
        co_await sim.timeout(1e6);
    }
}

void DeficitRoundRobin::cleanup_for_job(unsigned jid) {
    active_jobs.erase(jid);
    ready_queue.erase(jid);
}

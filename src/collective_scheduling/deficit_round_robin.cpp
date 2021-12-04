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
        ready_pqueues[tensor->job->id].push(std::move(queue[tensor->key]));
        queue.erase(tensor->key);
        if (!loop_is_running) {
            collective_scheduler(sim, cluster);
            loop_is_running = true;
        }
    }
    co_await sim.timeout(0);
}

simcpp20::event<SIM_UNIT> DeficitRoundRobin::collective_scheduler(
        simcpp20::simulation<SIM_UNIT> &sim,
        Cluster &cluster) {
    std::unordered_map<unsigned, unsigned> quantums;
    while (!ready_pqueues.empty()) {
        for (auto it = ready_pqueues.begin(); it != ready_pqueues.end(); ++it) {
            auto jid = it->first;
            auto &pq = it->second;
            if (pq.empty()) {
                ready_pqueues.erase(it);
                continue;
            }
//            printf("2 %lu\n", pq.size());
            auto &tensors = pq.top();
//            printf("3 %zu\n", tensors.size());
            auto &front = tensors.front();
//            printf("4\n");
            std::set<unsigned> involved_wids{front->job->wids_allocated};
//            printf("5\n");
            std::set<unsigned> involved_jids{jid};
//            printf("6\n");
            while (true) { // work conservation
//                printf("7\n");
                unsigned current_min_quantum = 0xffffffff;
                auto selected_idx = it;
//                printf("8\n");
                std::set<unsigned> &selected_wids = front->job->wids_allocated;
//                printf("9\n");
                for (auto j = ready_pqueues.begin(); j != ready_pqueues.end(); ++j) {
                    auto job_id = j->first;
//                    printf("10\n");
                    if (involved_jids.contains(job_id) || ready_pqueues[job_id].empty()) continue;
//                    printf("11 %lu\n", ready_pqueues[job_id].size());
                    auto additional_tensors = ready_pqueues[job_id].top();
//                    printf("12 %u\n", quantums[job_id]);
                    if (cluster._topo->accommodate(involved_wids, additional_tensors.front()->job->wids_allocated) &&
                        quantums[job_id] < current_min_quantum) {
//                        printf("13\n");
                        current_min_quantum = quantums[job_id];
//                        printf("14\n");
                        selected_idx = j;
//                        printf("15\n");
                        selected_wids = additional_tensors.front()->job->wids_allocated;
                    }
//                    printf("16\n");
                }
                if (selected_idx == it) {
//                    printf("20\n");
                    break; // nothing can be added
                }
//                printf("17\n");
                involved_jids.insert(selected_idx->first);
//                printf("18\n");
                for (auto wid: selected_wids) involved_wids.insert(wid);
//                printf("19\n");
            }
            std::vector<simcpp20::event<SIM_UNIT>> allreduce_events{};
            for (auto job_id: involved_jids) {
                auto all_tensors = ready_pqueues[job_id].top();
                auto front_tensor = ready_pqueues[job_id].top().front();
                quantums[job_id] += 1;
                myprintf(7, "invoking allreduce for job_id %u tid %u iter %u cid %u/%u\n", job_id, front_tensor->tensor_id,
                         front_tensor->iter, front_tensor->allreduced_size / CHUNK_SIZE + 1,
                         front_tensor->size % CHUNK_SIZE ? front_tensor->size / CHUNK_SIZE + 1 : front_tensor->size /
                                                                                                 CHUNK_SIZE);
                if (front_tensor->allreduced_size + CHUNK_SIZE >= front_tensor->size) {
                    myprintf(7, "popping job_id %u tid %u\n", job_id, front_tensor->tensor_id);
                    ready_pqueues[job_id].pop();
                }
                for (auto tensor:all_tensors) {
                    allreduce_events.push_back(std::move(tensor->machine->allreduce(sim, tensor, CHUNK_SIZE)));
                }
            }
            co_await sim.all_of(allreduce_events);
        }
    }
//    printf("dddssssdd\n");
    co_await sim.timeout(0);
    loop_is_running = false;
}

void DeficitRoundRobin::cleanup_for_job(unsigned jid) {
//    myprintf("clean invoked\n");
//    while(!ready_queue[jid].empty()) ready_queue[jid].pop();
    ready_pqueues.erase(jid);
}

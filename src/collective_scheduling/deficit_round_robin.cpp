#include "deficit_round_robin.h"
#include "job.h"
#include "cluster.h"
#include "worker.h"

void DeficitRoundRobin::update_maps() {
    for (unsigned i = 0; i < ready_pqueues.size(); ++i) {
        jid_to_deque_idx[ready_pqueues[i].first] = i;
    }
}

void DeficitRoundRobin::print_ready_pqueues_info() {
    for (unsigned i = 0; i < ready_pqueues.size(); ++i) {
        myprintf(7, "index %d jid %d\n", i, ready_pqueues[i].first);
    }
}

inline bool DeficitRoundRobin::ready_pqueues_all_empty() const {
    return std::all_of(ready_pqueues.cbegin(), ready_pqueues.cend(),
                       [](auto &pair) { return pair.second.empty(); });
}

simcpp20::event<SIM_UNIT>
DeficitRoundRobin::enqueue(simcpp20::simulation<SIM_UNIT> &sim, Tensor *tensor) {
    queue[tensor->key].push_back(tensor);
    if (queue[tensor->key].size() == tensor->job->num_workers_allocated) {
        myprintf(7, "putting jid %u tid %u size %u into ready queue %d\n",
                 tensor->job->id, tensor->tensor_id, tensor->size, loop_is_running);
        if (!jid_to_deque_idx.contains(tensor->job->id)) {
            jid_to_deque_idx[tensor->job->id] = ready_pqueues.size();
            ready_pqueues.emplace_back();
        }
        ready_pqueues[jid_to_deque_idx[tensor->job->id]].first = tensor->job->id;
        ready_pqueues[jid_to_deque_idx[tensor->job->id]].second.push(std::move(queue[tensor->key]));
        queue.erase(tensor->key);
        print_ready_pqueues_info();
        if (!loop_is_running) {
            collective_scheduler(sim, cluster);
            loop_is_running = true;
        }
    }
    co_await sim.timeout(0);
}

simcpp20::event<SIM_UNIT> DeficitRoundRobin::collective_scheduler(
        simcpp20::simulation<SIM_UNIT> &sim, Cluster &cluster) {
    std::unordered_map<unsigned, unsigned> quantums;
    while (!ready_pqueues_all_empty()) {
        for (unsigned i = 0; i < ready_pqueues.size(); ++i) {
            auto &pq = ready_pqueues[i].second;
            if (pq.empty()) {
                continue;
            }
            auto &tensors = pq.top();
            auto &front = tensors.front();
            auto jid = ready_pqueues[i].first;
            std::set<unsigned> involved_wids{front->job->wids_allocated};
            std::set<unsigned> involved_jids{jid};
            while (true) { // work conservation
                unsigned current_min_quantum = 0xffffffff;
                auto selected_idx = i;
                std::set<unsigned> selected_wids = front->job->wids_allocated;
                for (auto j = i; j < ready_pqueues.size(); ++j) {
                    auto job_id = ready_pqueues[j].first;
                    if (involved_jids.contains(job_id) || ready_pqueues[j].second.empty()) continue;
                    auto additional_tensors = ready_pqueues[j].second.top();
                    if (cluster._topo->accommodate(involved_wids, additional_tensors.front()->job->wids_allocated)
                        && quantums[job_id] < current_min_quantum) {
                        current_min_quantum = quantums[job_id];
                        selected_idx = j;
                        selected_wids = additional_tensors.front()->job->wids_allocated;
                    }
                }
                if (selected_idx == i) {
                    break; // nothing can be added
                }
                involved_jids.insert(ready_pqueues[selected_idx].first);
                for (auto wid: selected_wids) involved_wids.insert(wid);
            }
            std::vector<simcpp20::event<SIM_UNIT>> allreduce_events{};
            for (auto job_id: involved_jids) {
                auto all_tensors = ready_pqueues[jid_to_deque_idx[job_id]].second.top();
                auto front_tensor = ready_pqueues[jid_to_deque_idx[job_id]].second.top().front();
                quantums[job_id] += 1;
                myprintf(7, "invoking allreduce for job_id %u tid %u iter %u cid %u/%u\n",
                         job_id, front_tensor->tensor_id,
                         front_tensor->iter, front_tensor->allreduced_size / CHUNK_SIZE + 1,
                         front_tensor->size % CHUNK_SIZE ? front_tensor->size / CHUNK_SIZE + 1
                                                         : front_tensor->size / CHUNK_SIZE);
                if (front_tensor->allreduced_size + CHUNK_SIZE >= front_tensor->size) {
                    myprintf(7, "popping job_id %u tid %u\n", job_id, front_tensor->tensor_id);
                    ready_pqueues[jid_to_deque_idx[job_id]].second.pop();
                }
                for (auto tensor:all_tensors) {
                    allreduce_events.push_back(std::move(tensor->machine->allreduce(sim, tensor, CHUNK_SIZE)));
                }
            }
            co_await sim.all_of(allreduce_events);
        }
//        for (auto idx: pq_idx_to_remove) { // all stored negative indices are implicitly sorted
//            printf("remove index %d\n", idx);
//            if (ready_pqueues.size()==1) ready_pqueues.clear();
//            else ready_pqueues.erase(ready_pqueues.begin()-idx);
//        }
    }
    co_await sim.timeout(0);
    loop_is_running = false;
}


void DeficitRoundRobin::cleanup_for_job(unsigned jid) {
//    while(!ready_queue[jid].empty()) ready_queue[jid].pop();
//    ready_pqueues.erase(jid);
    for (auto it = ready_pqueues.begin(); it != ready_pqueues.end();) {
        if (it->first == jid) {
            it = ready_pqueues.erase(it);
        } else {
            ++it;
        }
    }
    jid_to_deque_idx.erase(jid);
    update_maps();
}

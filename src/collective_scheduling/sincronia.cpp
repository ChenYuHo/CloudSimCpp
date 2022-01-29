#include "sincronia.h"
#include "job.h"
#include "cluster.h"
#include "worker.h"
#include "topology.h"

//void Sincronia::print_ready_pqueues_info() {
//    for (unsigned i = 0; i < ready_pqueues.size(); ++i) {
//        myprintf(7, "index %d jid %d\n", i, ready_pqueues[i].first);
//    }
//}

inline bool Sincronia::ready_pqueues_all_empty() const {
    return std::all_of(ready_pqueues.cbegin(), ready_pqueues.cend(),
                       [](auto &pair) { return pair.second.empty(); });
}

simcpp20::event<SIM_UNIT>
Sincronia::enqueue(simcpp20::simulation<SIM_UNIT> &sim, Tensor *tensor) {
    queue[tensor->key].push_back(tensor);
    if (queue[tensor->key].size() % tensor->job->num_workers_allocated == 0) {
        myprintf(7, "putting jid %u tid %u size %u into ready queue %d\n",
                 tensor->job->id, tensor->tensor_id, tensor->size, loop_is_running);
        ready_pqueues[tensor->job->id].push(tensor);
//        print_ready_pqueues_info();
        if (!loop_is_running) {
            loop_is_running = true;
            collective_scheduler(sim, cluster);
        }
    }
    co_await sim.timeout(0);
}

uint64_t get_weight(const Tensor *tensor) {
    if (tensor->tensor_id > ((1+tensor->job->fp_layer) % tensor->job->model.size())) {
        return tensor->job->model[tensor->job->fp_layer];
    } else { // same layer
        return tensor->job->model[tensor->job->fp_layer] - tensor->allreduced_size;
    }
}

simcpp20::event<SIM_UNIT> Sincronia::collective_scheduler(
        simcpp20::simulation<SIM_UNIT> &sim, Cluster &cluster) {
    while (!ready_pqueues_all_empty()) {
        std::unordered_map<Tensor *, uint64_t> weights{};
        for (auto &pair: ready_pqueues) {
            auto &pq = pair.second;
            if (!pq.empty()) {
                Tensor *tensor = pq.top();
                while (can_erase.contains(tensor->key)) {
                    myprintf(7, "popping jid %u tid %u\n", tensor->job->id, tensor->tensor_id);
                    can_erase.erase(tensor->key);
                    pq.pop();
                    for (unsigned i = 0; i < tensor->job->wids_allocated.size(); ++i) {
                        queue[tensor->key].pop_front();
                    }
                    if (pq.empty()) {
                        break;
                    }
                    tensor = pq.top();
                }
                if (pq.empty()) continue;
                weights[tensor] = get_weight(tensor);
            }
        }

        // bssi order, sequential with work conservation
        auto result = (weights.size() == 1) ? std::deque<uint64_t>{weights.cbegin()->first->key}
                                            : cluster._topo->bssi(weights);

        static unsigned counter = 0;
        while (!result.empty()) {
            myprintf(7, "while %u\n", counter++);
            for (auto &key: result) {
                auto &tensor = queue[key].front();
                myprintf(7, "result jid %u tid %u\n", tensor->job->id, tensor->tensor_id);
            }
            std::vector<simcpp20::event<SIM_UNIT>> allreduce_events{};
            auto front_tkey = result.front();
            auto tensor = queue[front_tkey].front();
            result.pop_front();
            std::set<unsigned> involved_wids{tensor->job->wids_allocated};
            std::set<uint64_t> involved_tkeys{tensor->key};
            for (auto &key : result) { // work conservation
                auto that = queue[key].front();
                auto &wids_allocated = that->job->wids_allocated;
                if (cluster._topo->accommodate(involved_wids, wids_allocated)) {
                    involved_tkeys.insert(key);
                    for (auto wid: wids_allocated) involved_wids.insert(wid);
                }
            }
            std::erase_if(result, [&involved_tkeys](uint64_t key) { return involved_tkeys.contains(key); });
            for (auto key: involved_tkeys) {
                auto &front_tensor = queue[key].front();
                myprintf(7, "%p jid %u tid %u\n", front_tensor, front_tensor->job->id, front_tensor->tensor_id);
                if (front_tensor->allreduced_size + CHUNK_SIZE >= front_tensor->size) {
                    myprintf(7, "can erase %p jid %u tid %u\n",
                             front_tensor, front_tensor->job->id, front_tensor->tensor_id);
                    can_erase.insert(front_tensor->key);
                }
                for (auto t: queue[key]) {
                    allreduce_events.push_back(std::move(t->machine->allreduce(sim, t, CHUNK_SIZE)));
                }
            }
            co_await sim.all_of(allreduce_events);
        }
    }
    co_await sim.timeout(0);
    loop_is_running = false;
}


void Sincronia::cleanup_for_job(unsigned jid) {
    std::erase_if(ready_pqueues, [jid](auto &pair) { return pair.first == jid; });
//    update_maps();
}

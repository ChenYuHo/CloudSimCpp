#include "sincronia.h"
#include "job.h"
#include "cluster.h"
#include "worker.h"
#include "common.h"
#include <glog/logging.h>

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
        DVLOG(1) << fmt::format("putting jid {} tid {} size {} into ready queue {}\n",
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

double get_weight(const Tensor *tensor) {
    if (tensor->tensor_id > ((1 + tensor->job->fp_layer) % tensor->job->model.size())) {
        return double(tensor->job->model[tensor->job->fp_layer]);
    } else { // same layer
        return double(tensor->job->model[tensor->job->fp_layer] - tensor->allreduced_size);
    }
}

simcpp20::event<SIM_UNIT> Sincronia::collective_scheduler(
        simcpp20::simulation<SIM_UNIT> &sim, Cluster &cluster) {
    while (!ready_pqueues_all_empty()) {
        co_await sim.timeout(timeFromUs(8000000u/HOST_NIC)); // time for ~1000KB transfer
        std::unordered_map<Tensor *, double> weights{}; // to be scheduled on
        for (auto &pair: ready_pqueues) { // pair: jid -> pq
            auto &pq = pair.second;
            // clean out can_erase
            while (!pq.empty() && can_erase.contains(pq.top()->key)) {
                Tensor *tensor = pq.top();
                DVLOG(1) << fmt::format("popping jid {} tid {}", tensor->job->id, tensor->tensor_id);
                can_erase.erase(tensor->key);
                pq.pop();
                for (unsigned i = 0; i < tensor->job->num_workers_allocated; ++i) {
                    queue[tensor->key].pop_front();
                }
            }
            if (pq.empty()) continue;
            // take one into weights
            weights[pq.top()] = get_weight(pq.top());
        }
        if (weights.empty()) continue;

        // bssi order
        auto result = (weights.size() == 1) ? std::deque<uint64_t>{weights.cbegin()->first->key}
                                            : cluster._topo->bssi(weights);
        while (!result.empty()) { // sequentially, with work conservation
#ifndef NDEBUG
            for (auto &key: result) {
                auto &tensor = queue[key].front();
                myprintf(7, "result jid %u tid %lu\n", tensor->job->id, tensor->tensor_id);
            }
#endif
            std::vector<simcpp20::event<SIM_UNIT>> allreduce_events{};
            auto front_tkey = result.front();
            auto tensor = queue[front_tkey].front();
            result.pop_front();
            std::unordered_set<unsigned> involved_wids{tensor->job->wids_allocated};
            std::unordered_set<uint64_t> involved_tkeys{tensor->key};
            for (auto &key: result) { // work conservation
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
                myprintf(7, "%p jid %u tid %lu\n", front_tensor, front_tensor->job->id, front_tensor->tensor_id);
                if (front_tensor->allreduced_size + CHUNK_SIZE >= front_tensor->size) {
                    myprintf(7, "can erase %p jid %u tid %lu\n",
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
//    co_await sim.timeout(0);
    loop_is_running = false;
}

void Sincronia::cleanup_for_job(unsigned jid) {
    std::erase_if(ready_pqueues, [jid](auto &pair) { return pair.first == jid; });
//    update_maps();
}

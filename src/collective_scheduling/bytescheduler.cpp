#include "bytescheduler.h"
#include "job.h"
#include "cluster.h"
#include "worker.h"

simcpp20::event<SIM_UNIT>
ByteScheduler::enqueue(simcpp20::simulation<SIM_UNIT> &sim, Tensor *tensor) {
    queue[tensor->key].push_back(tensor);
    if (queue[tensor->key].size() == tensor->job->num_workers_allocated) {
        myprintf(7, "putting jid %u tid %u size %u into ready queue\n", tensor->job->id, tensor->tensor_id,
                 tensor->size);
        ready_queue[tensor->job->id].push(tensor);
        if (!active_jobs.contains(tensor->job->id)) {
            active_jobs.insert(tensor->job->id);
            kick_off(sim, tensor->job->id);
        }
    }
    co_await sim.timeout(0);
}

simcpp20::event<SIM_UNIT> ByteScheduler::kick_off(simcpp20::simulation<SIM_UNIT> &sim, unsigned jid) {
    auto &pq = ready_queue.at(jid);
    while (!pq.empty()) {
        const auto &tensor = pq.top();
        const auto key = tensor->key;
        if (tensor->allreduced_size + CHUNK_SIZE >= tensor->size) {
            myprintf(7, "popping jid %u tid %u\n", jid, tensor->tensor_id);
            pq.pop();
        }
        std::vector<simcpp20::event<SIM_UNIT>> allreduce_events{};
        myprintf(7, "invoking allreduce for jid %u tid %u iter %u cid %u/%u\n",
                 jid, tensor->tensor_id, tensor->iter, tensor->allreduced_size / CHUNK_SIZE,
                 tensor->size % CHUNK_SIZE ? tensor->size / CHUNK_SIZE + 1 : tensor->size / CHUNK_SIZE);
        for (const auto &t: queue[key]) {
            allreduce_events.push_back(std::move(t->machine->allreduce(sim, t, CHUNK_SIZE)));
        }
        co_await sim.all_of(allreduce_events);
    }
    active_jobs.erase(jid);
    co_await sim.timeout(0);
}

simcpp20::event<SIM_UNIT> ByteScheduler::collective_scheduler(
        simcpp20::simulation<SIM_UNIT> &sim,
        Cluster &cluster) {
    co_await sim.timeout(0);
}

void ByteScheduler::cleanup_for_job(unsigned jid) {
    std::erase_if(queue, [jid](const auto& item){
        auto const& [key, tensors] = item;
        return !tensors.empty() && (tensors.front()->job->id == jid);
    });
    active_jobs.erase(jid);
    ready_queue.erase(jid);
}

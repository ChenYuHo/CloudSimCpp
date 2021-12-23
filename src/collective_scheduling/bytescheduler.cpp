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
            myprintf(7, "kick off jid %d loop!\n", tensor->job->id);
        }
    }
    co_await sim.timeout(0);
}

simcpp20::event<SIM_UNIT> ByteScheduler::kick_off(simcpp20::simulation<SIM_UNIT> &sim, unsigned jid) {
    auto &pq = ready_queue.at(jid);
    while (!pq.empty() && active_jobs.contains(jid)) {
        const auto &tensor = pq.top();
        const auto key = tensor->key;
        std::vector<simcpp20::event<SIM_UNIT>> allreduce_events{};
        auto tid = tensor->tensor_id;
        auto iter = tensor->iter;
        auto cid = (tensor->allreduced_size / CHUNK_SIZE)+1;
        auto total_cid = tensor->size % CHUNK_SIZE ? tensor->size / CHUNK_SIZE + 1 : tensor->size / CHUNK_SIZE;
        myprintf(7, "%llu invoking allreduce for jid %u tid %u iter %u cid %u/%u\n", sim.now(),
                 jid, tid, iter, cid, total_cid);
        for (auto t: queue[key]) {
            allreduce_events.push_back(std::move(t->machine->allreduce(sim, t, CHUNK_SIZE)));
        }
        if (cid >= total_cid) {
            myprintf(7, "popping jid %u tid %u\n", jid, tid);
            pq.pop();
            queue[key].clear();
        }
        co_await sim.all_of(allreduce_events);
        myprintf(7, "%llu done allreduce for jid %u tid %u iter %u cid %u/%u\n", sim.now(),
                 jid, tid, iter, cid, total_cid);
    }
    myprintf(7, "%llu jid %d has no tensors at the moment!\n", sim.now(), jid);
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

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
        ready_queue[tensor->job->id].push(std::move(queue[tensor->key]));
        queue.erase(tensor->key);
        if (!active_jobs.contains(tensor->job->id)) { //  && !done_jobs.contain(tensor->job->id)
            active_jobs.insert(tensor->job->id);
            kick_off(sim, tensor->job->id);
        }
    }
    co_await sim.timeout(0);
}

simcpp20::event<SIM_UNIT> ByteScheduler::kick_off(simcpp20::simulation<SIM_UNIT> &sim, unsigned jid) {
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

simcpp20::event<SIM_UNIT> ByteScheduler::collective_scheduler(
        simcpp20::simulation<SIM_UNIT> &sim,
        Cluster &cluster) {
    co_await sim.timeout(timeFromSec(1));
}

void ByteScheduler::cleanup_for_job(unsigned jid) {
    active_jobs.erase(jid);
    ready_queue.erase(jid);
}

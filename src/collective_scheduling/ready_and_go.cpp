#include "ready_and_go.h"
#include "job.h"
#include "cluster.h"
#include "worker.h"

simcpp20::event<SIM_UNIT> ReadyAndGo::collective_scheduler(
        simcpp20::simulation<SIM_UNIT> &sim,
        Cluster &cluster) {
    co_await sim.timeout(0);
}

simcpp20::event<SIM_UNIT> ReadyAndGo::enqueue(simcpp20::simulation<SIM_UNIT> &sim, Tensor *tensor) {
    queue[tensor->key].push_back(tensor);
    if (queue[tensor->key].size() == tensor->job->num_workers_allocated) {
        job_locks.try_emplace(tensor->job->id, sim, 1);
        co_await job_locks.at(tensor->job->id).request();
        vector<simcpp20::event<SIM_UNIT>> allreduce_events{};
        for (auto &t: queue[tensor->key]) {
            allreduce_events.push_back(std::move(t->machine->allreduce(sim, t)));
        }
        co_await sim.all_of(allreduce_events);
        job_locks.at(tensor->job->id).release();
        queue.erase(tensor->key);
    }
    co_await sim.timeout(0);
}


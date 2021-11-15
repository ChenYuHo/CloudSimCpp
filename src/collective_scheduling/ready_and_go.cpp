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
        for (auto &t: queue[tensor->key]) {
            t->machine->allreduce(sim, t);
        }
        queue.erase(tensor->key);
    }
    co_await sim.timeout(0);
}


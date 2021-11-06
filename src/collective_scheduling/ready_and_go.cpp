#include "ready_and_go.h"
#include "job.h"
#include "cluster.h"
#include "worker.h"

simcpp20::event<SIM_UNIT> ReadyAndGo::enqueue(simcpp20::simulation<SIM_UNIT> & sim, std::shared_ptr<Tensor> tensor) {
    auto key = std::make_pair(tensor->job->id, tensor->tensor_id);
    queue[key].push_back(tensor);
    if (queue[key].size() == tensor->job->num_workers_allocated) {
        for(auto & t: queue[key]) {
//            myprintf("Allreduce go %p\n", t.get());
            t->machine->allreduce(sim, t);
        }
        queue[key].clear();
    }
    co_await sim.timeout(0);
}

simcpp20::event<SIM_UNIT> ReadyAndGo::collective_scheduler(
        simcpp20::simulation<SIM_UNIT> &sim,
        Cluster &cluster) {
    co_await sim.timeout(0);
}


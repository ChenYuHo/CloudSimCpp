#include "first_in_first_out_one_by_one.h"
#include "job.h"
#include "cluster.h"
#include "worker.h"

simcpp20::event<SIM_UNIT>
FirstInFirstOutOneByOne::enqueue(simcpp20::simulation<SIM_UNIT> &sim, const shared_ptr<Tensor> &tensor) {
    auto key = std::make_pair(tensor->job->id, tensor->tensor_id);
    queue[key].push_back(tensor);
    std::vector<simcpp20::event<SIM_UNIT>> events{};
    if (queue[key].size() == tensor->job->num_workers_allocated) {
        co_await lock.request();
        for(auto & t: queue[key]) {
//            myprintf("Allreduce go %p\n", t.get());
            events.push_back(t->machine->allreduce(sim, t));
        }
        queue[key].clear();
        co_await sim.all_of(events);
        lock.release();
    }
}

simcpp20::event<SIM_UNIT> FirstInFirstOutOneByOne::collective_scheduler(
        simcpp20::simulation<SIM_UNIT> &sim,
        Cluster &cluster) {
    co_await sim.timeout(0);
}



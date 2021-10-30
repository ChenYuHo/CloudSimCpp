#include "first_in_first_out_one_by_one.h"
#include "job.h"
#include "cluster.h"
#include "worker.h"

void FirstInFirstOutOneByOne::enqueue(const std::shared_ptr<Tensor> &tensor) {
    auto key = std::make_pair(tensor->job->id, tensor->tensor_id);
    queue[key].push_back(tensor);
    if (queue[key].size() == tensor->job->num_workers_allocated) {
        ready_queue.push(std::move(queue[key]));
        queue.erase(key);
    }
}

simcpp20::event<SIM_UNIT> FirstInFirstOutOneByOne::run_scheduler_once(
        simcpp20::simulation<SIM_UNIT> &sim,
        Cluster &cluster) {
    if (!ready_queue.empty()) {
        auto &tensors = ready_queue.front();
        std::vector<simcpp20::event<SIM_UNIT>> allreduce_events{};
        for (auto &tensor : tensors) {
//            myprintf("[%llu]\tinvoking allreduce within cs %p s%llu j%d m%d\n", sim.now(),
//                   tensor.get(), tensor->size, tensor->job->id, tensor->machine->id);
            allreduce_events.push_back(tensor->machine->allreduce(sim, tensor));
        }
//        auto begin = sim.now();
        co_await sim.all_of(allreduce_events);
//        auto end = sim.now();
        ready_queue.pop();
    }
}


simcpp20::event<SIM_UNIT> FirstInFirstOutOneByOne::collective_scheduler(
        simcpp20::simulation<SIM_UNIT> &sim,
        Cluster &cluster) {
    while (!cluster.all_jobs_finished) {
        co_await run_scheduler_once(sim, cluster);
        co_await sim.timeout(1e6);
    }
}

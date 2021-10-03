//
// Created by Chen-Yu Ho on 10/3/21.
//

#include "first_in_first_out_one_by_one.h"
#include "job.h"
#include "cluster.h"
#include "worker.h"

void FirstInFirstOutOneByOne::enqueue(const std::shared_ptr<Tensor> &tensor) {
    auto key = std::make_pair(tensor->job->id, tensor->tensor_id);
    queue[key].push_back(tensor);
//        printf("queue size %zu %d %d\n", queue[key].size(), tensor->tensor_id, tensor->job->id);
    if (queue[key].size() == tensor->job->num_workers_allocated) {
        ready_queue.push(std::move(queue[key]));
        queue.erase(key);
    }
}

simcpp20::event<SIM_UNIT> FirstInFirstOutOneByOne::collective_scheduler(
        simcpp20::simulation<SIM_UNIT> &sim,
        Cluster &cluster) {
    while (!cluster.all_jobs_finished) {
        if (!ready_queue.empty()) {
            auto &tensors = ready_queue.front();
            std::vector<simcpp20::event<SIM_UNIT>> allreduce_events;
            for (auto &tensor : tensors) {
//                    printf("[%llu]\tinvoking allreduce within cs s%llu j%d m%d\n", sim.now(),
//                           tensor->size, tensor->job->id, tensor->machine->id);
                allreduce_events.push_back(
                        tensor->machine->allreduce(sim, tensor->size, std::to_string(tensor->tensor_id),
                                                   tensor->job));
            }
            auto begin = sim.now();
            co_await sim.all_of(allreduce_events);
            auto end = sim.now();
            for (auto &tensor : tensors) {
//                if (PRINT) {
                printf("[allreduce] iter %d jid %d mid %d tid %d size %d start %llu duration %llu end %llu\n",
                       tensor->iter, tensor->job->id, tensor->machine->id, tensor->tensor_id, tensor->size,
                       begin,
                       end - begin, end);
//                }
                tensor->iter++;
                tensor->lock.release();
            }
            ready_queue.pop();
        }
        co_await sim.timeout(1e6);
    }
}


#include "first_in_first_out_one_by_one.h"
#include "job.h"
#include "cluster.h"
#include "worker.h"

void FirstInFirstOutOneByOne::enqueue(const std::shared_ptr<Tensor> &tensor) {
    auto key = std::make_pair(tensor->job->id, tensor->tensor_id);
    queue[key].push_back(tensor);
    printf("queue size %zu %llu %d\n", queue[key].size(), tensor->tensor_id, tensor->job->id);
    if (queue[key].size() == tensor->job->num_workers_allocated) {
        printf("ppp\n");
        ready_queue.push(std::move(queue[key]));
        run_scheduler_once(sim, cluster);
        queue.erase(key);
    }
}

simcpp20::event<SIM_UNIT> FirstInFirstOutOneByOne::run_scheduler_once(
        simcpp20::simulation<SIM_UNIT> &sim,
        Cluster &cluster) {
    if (!ready_queue.empty()) {
//        printf("aaa\n");
        auto &tensors = ready_queue.front();
//        printf("bbb\n");
        std::vector<simcpp20::event<SIM_UNIT>> allreduce_events{};

//        printf("ccc\n");
        for (auto &tensor : tensors) {
//            printf("ddd\n");
            printf("[%llu]\tinvoking allreduce within cs s%llu j%d m%d\n", sim.now(),
                   tensor->size, tensor->job->id, tensor->machine->id);
//            printf("eee\n");
//            allreduce_events.push_back(
//                    tensor->machine->allreduce(sim, tensor->size, std::to_string(tensor->tensor_id),
//                                               tensor->job));
//            printf("fff\n");
        }
//        printf("ggg\n");
        auto begin = sim.now();
//            printf("w1\n");
        co_await sim.all_of(allreduce_events);
//            printf("w2\n");
//        printf("hhh\n");
        auto end = sim.now();
        for (auto &tensor : tensors) {
//                if (PRINT) {
//            printf("[allreduce] iter %d jid %d mid %d tid %d size %d start %llu duration %llu end %llu\n",
//                   tensor->iter, tensor->job->id, tensor->machine->id, tensor->tensor_id, tensor->size,
//                   begin,
//                   end - begin, end);
////                }
            tensor->iter++;
//            printf("iii\n");
            tensor->lock.release();
//            printf("jjj\n");
        }
        ready_queue.pop();
    } //else printf("ready queue is empty\n");
}


simcpp20::event<SIM_UNIT> FirstInFirstOutOneByOne::collective_scheduler(
        simcpp20::simulation<SIM_UNIT> &sim,
        Cluster &cluster) {
    while (!cluster.all_jobs_finished) {
        run_scheduler_once(sim, cluster);
        co_await sim.timeout(1e6);
    }
}


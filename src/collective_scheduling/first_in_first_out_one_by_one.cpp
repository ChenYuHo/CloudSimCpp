#include "first_in_first_out_one_by_one.h"
#include "job.h"
#include "cluster.h"
#include "worker.h"

simcpp20::event<SIM_UNIT> FirstInFirstOutOneByOne::collective_scheduler(
        simcpp20::simulation<SIM_UNIT> &sim,
        Cluster &cluster) {
    co_await sim.timeout(0);
}

simcpp20::event<SIM_UNIT> FirstInFirstOutOneByOne::enqueue(simcpp20::simulation<SIM_UNIT> &sim, Tensor *tensor) {
    // a switch runs one job at a time
    queue[tensor->key].push_back(tensor);
    if (queue[tensor->key].size() == tensor->job->num_workers_allocated) {
        for (auto &t: queue[tensor->key]) {
            t->machine->allreduce(sim, t);
        }
        ready_queue.push(std::move(queue[tensor->key]));
        queue.erase(tensor->key);
    }
    co_await sim.timeout(0);


    auto key = get_key(tensor);
    queue[key].push_back(tensor);
    static bool flag = false;
    if (queue[key].size() == tensor->job->num_workers_allocated) {
        myprintf("putting tid %u jid %u into ready queue\n", tensor->tensor_id, tensor->job->id);
//        ready_queue[tensor->job->id].push(std::move(queue[key]));

        queue.erase(key);
//        if (!flag) {
//            flag = true;
//            auto &q = ready_queue.top();
//            auto &t = q.front();
//            myprintf("TEST %u %u\n", t->allreduced_size, t->size);
//            while (t->allreduced_size < t->size) {
//                for (auto &tt: q) {
//                    myprintf("invoking allreduce\n");
//                    co_await tt->machine->allreduce(sim, tt, CHUNK_SIZE);
//                    myprintf("TEST2 %u %u\n", tt->allreduced_size, tt->size);
//                }
//            }
//        }
//        run_scheduler_once(sim);
    }
    co_await sim.timeout(0);

}

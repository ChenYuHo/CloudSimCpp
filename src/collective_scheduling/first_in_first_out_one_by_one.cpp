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
    // a link (worker-switch) runs one job at a time
    queue[tensor->key].push_back(tensor);
    std::vector<simcpp20::event<simtime_picosec>> events;
    if (queue[tensor->key].size() == tensor->job->num_workers_allocated) {
        co_await lock.request(); // IDEA: get locks from topology, lock all so that one link only processes one allreduce at a time
        for (auto &t: queue[tensor->key]) {
            events.push_back(t->machine->allreduce(sim, t));
        }
//        ready_queue.push(std::move(queue[tensor->key]));
        queue.erase(tensor->key);
        co_await sim.all_of(events);
        lock.release();
    }
//    co_await sim.timeout(0);
}

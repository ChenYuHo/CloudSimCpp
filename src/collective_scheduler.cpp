//
// Created by Chen-Yu Ho on 10/3/21.
//

//#include "collective_scheduler.h"
//#include "job.h"
//#include "worker.h"

//simcpp20::event<SIM_UNIT>
//CollectiveScheduler::enqueue(simcpp20::simulation<SIM_UNIT> &sim,
//                             Tensor *tensor) {
//    auto key = get_key(tensor);
//    queue[key].push_back(tensor);
//    if (queue[key].size() == tensor->job->num_workers_allocated) {
//        ready_queue.push(std::move(queue[key]));
//        queue.erase(key);
//    }
//    co_await sim.timeout(0);
//}

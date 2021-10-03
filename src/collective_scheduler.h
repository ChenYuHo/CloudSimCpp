//
// Created by Chen-Yu Ho on 10/3/21.
//

#ifndef CLOUDSIMCPP_COLLECTIVE_SCHEDULER_H
#define CLOUDSIMCPP_COLLECTIVE_SCHEDULER_H
#include <memory>
#include "common.h"
class Cluster;

class CollectiveScheduler {
public:
//    std::map<unsigned, std::deque<pkt>> queues{}; // each job has a queue
//    virtual void enqueue(uint64_t size, Worker *machine, unsigned tensor_id, unsigned chunk_id,
//                         const std::shared_ptr<Job> &job) = 0;
    virtual void enqueue(const std::shared_ptr<Tensor> &) = 0;
//    {
//        auto it = queues.find(job->id);
//        if (it == queues.end()) {
//            queues.emplace(job->id, std::deque<pkt>{pkt{size, machine, tensor_id, chunk_id, job}});
//        } else {
//            auto &queue = queues[job->id];
//            queue.emplace_back(pkt{size, machine, tensor_id, chunk_id, job});
//        }
//        printf("queue size %zu\n", queues[job->id].size());
//    }

    virtual simcpp20::event<SIM_UNIT> collective_scheduler(simcpp20::simulation<SIM_UNIT> &, Cluster &) = 0;

};



#endif //CLOUDSIMCPP_COLLECTIVE_SCHEDULER_H

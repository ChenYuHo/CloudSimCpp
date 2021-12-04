//
// Created by Chen-Yu Ho on 11/17/21.
//

#ifndef CLOUDSIMCPP_DEFICIT_ROUND_ROBIN_H
#define CLOUDSIMCPP_DEFICIT_ROUND_ROBIN_H


#include <collective_scheduler.h>
#include <map>

class DeficitRoundRobin : public CollectiveScheduler {
public:
    DeficitRoundRobin(simcpp20::simulation<SIM_UNIT> &sim, Cluster &cluster)
    : CollectiveScheduler(sim, cluster) {};

    struct CustomCompare {
        bool
        operator()(const std::vector<Tensor *> &lhs,
                const std::vector<Tensor *> &rhs) {
            auto &l = lhs.front();
            auto &r = rhs.front();
            return l->iter == r->iter ? l->tensor_id > r->tensor_id : l->iter > r->iter;
        }
    };

    std::unordered_map<uint64_t, std::vector<Tensor *>> queue;
    std::map<unsigned, std::priority_queue<std::vector<Tensor *>,
        std::vector<std::vector<Tensor *>>, CustomCompare>> ready_pqueues{};
//    std::map<unsigned, std::priority_queue<std::vector<Tensor *>,
//        std::vector<std::vector<Tensor *>>, CustomCompare>> ready_pqueues{};

    simcpp20::event<SIM_UNIT> enqueue(simcpp20::simulation<SIM_UNIT> &, Tensor *) override;

    simcpp20::event<SIM_UNIT> collective_scheduler(simcpp20::simulation<SIM_UNIT> &sim,
                                                   Cluster &cluster) override;

    void cleanup_for_job(unsigned) override;

    std::unordered_map<unsigned, unsigned> vector_idx;

    bool loop_is_running = true;
};


#endif //CLOUDSIMCPP_DEFICIT_ROUND_ROBIN_H

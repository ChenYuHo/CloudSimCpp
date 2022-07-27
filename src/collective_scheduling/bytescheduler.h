#ifndef CLOUDSIMCPP_BYTESCHEDULER_H
#define CLOUDSIMCPP_BYTESCHEDULER_H

#include "collective_scheduler.h"
#include <queue>

class ByteScheduler : public CollectiveScheduler {
public:
    ByteScheduler(simcpp20::simulation<SIM_UNIT> &sim, Cluster &cluster)
            : CollectiveScheduler(sim, cluster) {};

    struct CustomCompare {
        bool operator()(const Tensor *lhs,
                        const Tensor *rhs) {
            return lhs->iter == rhs->iter ? lhs->tensor_id > rhs->tensor_id : lhs->iter > rhs->iter;
        }
    };

    std::unordered_map<uint64_t, std::vector<Tensor *>> queue{};

    std::unordered_map<unsigned, std::priority_queue<Tensor *,
            std::vector<Tensor *>, CustomCompare>> ready_queue{};

    simcpp20::event<SIM_UNIT> enqueue(simcpp20::simulation<SIM_UNIT> &, Tensor *) override;

    simcpp20::event<SIM_UNIT> collective_scheduler(simcpp20::simulation<SIM_UNIT> &sim,
                                                   Cluster &cluster) override;

    std::unordered_set<unsigned> active_jobs{};

    void cleanup_for_job(unsigned) override;

    simcpp20::event<SIM_UNIT> kick_off(simcpp20::simulation<SIM_UNIT> &, unsigned);

    std::unordered_set<unsigned> can_erase{};

//    simcpp20::event<SIM_UNIT> run_loop(simcpp20::simulation<SIM_UNIT> &sim, unsigned int jid);
//
//    simcpp20::event<SIM_UNIT> allreduce_one_chunk(simcpp20::simulation<SIM_UNIT> &sim, std::priority_queue<Tensor *,
//            std::vector<Tensor *>, CustomCompare> &);
};


#endif //CLOUDSIMCPP_BYTESCHEDULER_H

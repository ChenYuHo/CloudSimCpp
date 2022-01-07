#ifndef CLOUDSIMCPP_SINCRONIA_H
#define CLOUDSIMCPP_SINCRONIA_H


#include <collective_scheduler.h>
#include <map>

class Sincronia : public CollectiveScheduler {
public:
    Sincronia(simcpp20::simulation<SIM_UNIT> &sim, Cluster &cluster)
            : CollectiveScheduler(sim, cluster) {};

    struct CustomCompare {
        bool
        operator()(const Tensor *l,
                   const Tensor *r) {
            return l->iter == r->iter ? l->tensor_id > r->tensor_id : l->iter > r->iter;
        }
    };

    std::unordered_map<uint64_t, std::deque<Tensor *>> queue;
    std::unordered_map<
            unsigned,
            std::priority_queue<
                    Tensor *,
                    std::vector<Tensor *>,
                    CustomCompare
            >
    > ready_pqueues{}; // each pq: a job
    simcpp20::event<SIM_UNIT> enqueue(simcpp20::simulation<SIM_UNIT> &, Tensor *) override;

    simcpp20::event<SIM_UNIT> collective_scheduler(simcpp20::simulation<SIM_UNIT> &sim,
                                                   Cluster &cluster) override;


    void cleanup_for_job(unsigned) override;

    bool loop_is_running = true;

    void update_maps();

    void print_ready_pqueues_info();

    bool ready_pqueues_all_empty() const;

    std::set<unsigned> can_erase{};
};


#endif //CLOUDSIMCPP_SINCRONIA_H

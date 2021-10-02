//
// Created by Chen-Yu Ho on 9/29/21.
//
#include <algorithm>
#include "random.h"
#include "cluster.h"
#include "job.h"

std::unordered_map<unsigned, unsigned> RandomPlacement::place_job_in(
        std::shared_ptr<Cluster> cluster, std::shared_ptr<Job> job) {
    std::vector<unsigned> candidates;
    std::unordered_map<unsigned, unsigned> counter{};
    for (auto &worker: cluster->workers()) {
        for (int i = 0; i < worker->num_free_gpus(); i++) candidates.push_back(worker->id());
    }
    if (job->gpus_required > candidates.size()) return counter;
    std::vector<unsigned> selected;
    std::ranges::sample(candidates, std::back_inserter(selected), job->gpus_required, gen);
    for (auto machine_id: selected) {
        if (counter.find(machine_id) == counter.end()) {
            counter[machine_id] = 1;
        } else {
            counter[machine_id] += 1;
        }
    }
    return counter;
}

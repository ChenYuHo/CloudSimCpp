#include <unordered_map>
#include <algorithm>
#include "random.h"
#include "cluster.h"
#include "worker.h"
#include "job.h"

std::map<unsigned, unsigned> RandomPlacement::place_job_in(Cluster &cluster, Job *job) {
    std::vector<unsigned> candidates;
    std::map<unsigned, unsigned> counter;
    for (auto &machine:cluster.workers) {
        for (int i = 0; i < machine->gpu; i++) candidates.push_back(machine->id);
    }
    if (job->gpu > candidates.size()) return counter;
    std::vector<unsigned> selected;
    std::ranges::sample(candidates, std::back_inserter(selected), job->gpu, gen);
    for (auto machine_id:selected) {
        if (counter.find(machine_id) == counter.end()) {
            counter[machine_id] = 1;
        } else {
            counter[machine_id] += 1;
        }
    }
    return counter;
}

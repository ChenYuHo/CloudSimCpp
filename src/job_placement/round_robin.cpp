#include <unordered_map>
#include <algorithm>
#include "round_robin.h"
#include "cluster.h"
#include "worker.h"
#include "job.h"

std::map<unsigned, unsigned> RoundRobinPlacement::place_job_in(Cluster &cluster, Job *job) {
    for (auto &entry: some_map) {
        auto &tor = entry.first;
        auto &workers = entry.second;
    }


    std::vector<unsigned> candidates;
    std::map<unsigned, unsigned> counter{};
    unsigned available_machines = 0;
    for (auto &machine:cluster.workers) {
        if (machine->gpu > 0) available_machines++;
        for (int i = 0; i < machine->gpu; i++) candidates.push_back(machine->id);
    }
    if (job->gpu > candidates.size()) return counter;
    std::vector<unsigned> selected;
    do {
        selected.clear();
        std::ranges::sample(candidates, std::back_inserter(selected), job->gpu, gen);
    } while (force_distributed && available_machines > 1 && selected.size()==1);
    for (auto machine_id: selected) {
        counter[machine_id] += 1;
    }
    return counter;
}

#include "yarn.h"
#include "cluster.h"
#include "worker.h"
#include "switch.h"
#include "job.h"
#include <algorithm>

std::map<unsigned, unsigned> YARNPlacement::place_job_in(Cluster &cluster, Job *job) {
    // single worker
    myprintf(6, "Try allocating on a single worker\n");
    std::vector<unsigned> candidates{};
    std::map<unsigned, unsigned> counter{}; // worker_id -> allocated_GPUs
    for (auto &machine: *cluster.workers) {
        if (machine->gpu >= job->gpu) {
            for (unsigned i = 0; i < machine->gpu; ++i) candidates.push_back(machine->id);
        }
    }
    if (!candidates.empty()) {
        std::vector<unsigned> selected;
        // workers with more GPUs is more likely to be selected
        std::ranges::sample(candidates, std::back_inserter(selected), 1, gen);
        counter[selected[0]] = job->gpu;
        return counter;
    }


    // multi worker, same ToR
    myprintf(6, "Try allocating workers within the same ToR\n");
    candidates.clear();
    for (auto &tor: *cluster.switches) {
        unsigned free_gpus_in_tor = 0;
        for (auto &machine: tor->machines) {
            free_gpus_in_tor += machine->gpu;
        }
        if (free_gpus_in_tor >= job->gpu) {
            for (int i = 0; i < free_gpus_in_tor; ++i) candidates.push_back(tor->id);
        }
    }

    // ToR with more free GPUs is more likely to be selected
    if (!candidates.empty()) {
        // choose tor
        std::vector<unsigned> selected;
        std::ranges::sample(candidates, std::back_inserter(selected), 1, gen);
        candidates.clear();
        auto &tor = cluster.switch_map[selected[0]];
        // choose within tor
        for (auto &machine: tor->machines) {
            for (int i = 0; i < machine->gpu; ++i) candidates.push_back(machine->id);
        }
        selected.clear();
        // without replacement
        std::ranges::sample(candidates, std::back_inserter(selected), job->gpu, gen);
        for (auto machine_id: selected) {
            counter[machine_id] += 1;
        }
        return counter;
    }

    // neither single worker nor worker in the same ToR
    if (use_random_when_fail) {
        myprintf(6, "Fallback to random selection\n");
        candidates.clear();
        for (auto &machine: *cluster.workers) {
            for (int i = 0; i < machine->gpu; i++) candidates.push_back(machine->id);
        }
        if (job->gpu > candidates.size()) return counter; // not enough GPUs
        std::vector<unsigned> selected;
        // without replacement
        std::ranges::sample(candidates, std::back_inserter(selected), job->gpu, gen);
        for (auto machine_id: selected) {
            counter[machine_id] += 1;
        }
        return counter;
    } else return counter;
}

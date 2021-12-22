#include <unordered_map>
#include <algorithm>
#include "random.h"
#include "cluster.h"
#include "worker.h"
#include "switch.h"
#include "job.h"

bool multi_racks_placement(const std::vector<Worker *> &selected) {
    std::set<unsigned> placed_tors;
    for (const auto &machine: selected) {
        placed_tors.insert(machine->tor->id);
    }
    return placed_tors.size() > 1;
}

bool distributed_placement(const std::vector<Worker *> &selected) {
    std::set<unsigned> machines{};
    for (const auto &machine: selected) {
        machines.insert(machine->id);
    }
    return machines.size() > 1;
}

std::map<unsigned, unsigned> RandomPlacement::place_job_in(Cluster &cluster, Job *job) {
    std::vector<Worker *> candidates;
    std::map<unsigned, unsigned> counter{};
    unsigned available_machines = 0;
    std::set<unsigned> tors_with_available_machines;
    for (auto &machine:cluster.workers) {
        if (machine->gpu > 0) {
            available_machines++;
            tors_with_available_machines.insert(machine->tor->id);
        }
        for (int i = 0; i < machine->gpu; i++) candidates.push_back(machine);
    }
    if (job->gpu > candidates.size()) return counter; // not enough free GPUs
    std::vector<Worker *> selected;
    auto must_place_distributed = force_distributed && available_machines > 1 && job->gpu > 1;
    auto must_place_multi_racks = force_multi_racks && tors_with_available_machines.size() > 1 && job->gpu > 1;
    do {
        do {
            selected.clear();
            std::ranges::sample(candidates, std::back_inserter(selected), job->gpu, gen);
        } while (must_place_distributed && !distributed_placement(selected));
    } while (must_place_multi_racks && !multi_racks_placement(selected));
    for (auto machine: selected) {
        counter[machine->id] += 1;
    }
    return counter;
}
